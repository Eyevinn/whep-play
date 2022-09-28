#define GST_USE_UNSTABLE_API 1 // Removes compile warning

#include <csignal>
#include <cstdint>
#include <glib.h>
#include <gst/gst.h>
#include <gst/sdp/sdp.h>
#include <gst/webrtc/webrtc.h>
#include <iostream>
#include <libsoup/soup.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vector>

struct CustomData {

    GstElement* webrtc_source;
    GstElement* pipeline;
    GstElement* rtp_depay_vp8;
    GstElement* vp8_decoder;
    GstElement* sinkElement;
    std::string sdpOffer;
    std::string sdpAnswer;
    std::string location;
    std::string whepURL;

    CustomData()
        : webrtc_source(nullptr)
        , pipeline(nullptr)
        , rtp_depay_vp8(nullptr)
        , vp8_decoder(nullptr)
        , sinkElement(nullptr)
    {
    }

    ~CustomData()
    {

        printf("\nDestructing resources...\n");
        if (pipeline) {
            g_object_unref(pipeline);
        }
    }
};

GMainLoop* mainLoop = nullptr;
void padAddedHandler(GstElement* src, GstPad* pad, CustomData* data);
void onAnswerCreatedCallback(GstPromise* promise, gpointer userData);
void onRemoteDescSetCallback(GstPromise* promise, gpointer userData);
void onNegotiationNeededCallback(GstElement* src, CustomData* data);

void intSignalHandler(int32_t)
{

    g_main_loop_quit(mainLoop);
}

void handleSDPs(CustomData* data)
{

    GstSDPMessage* offerMessage;
    GstWebRTCSessionDescription* offerDesc;

    if (gst_sdp_message_new_from_text(data->sdpOffer.c_str(), &offerMessage) != GST_SDP_OK) {
        printf("Unable to create SDP object from offer\n");
    }

    offerDesc = gst_webrtc_session_description_new(GST_WEBRTC_SDP_TYPE_OFFER, offerMessage);
    if (!offerDesc) {
        printf("Unable to create SDP object from offer msg\n");
    }

    GstPromise* promiseRemote = gst_promise_new_with_change_func(onRemoteDescSetCallback, data, nullptr);
    if (!data->webrtc_source) {
        printf("webrtc_source is NULL\n");
    }

    g_signal_emit_by_name(data->webrtc_source, "set-remote-description", offerDesc, promiseRemote);
}

std::vector<std::string> getPostOffer(CustomData* data)
{

    SoupSession* session = soup_session_new();

    SoupMessage* msg = soup_message_new("POST", data->whepURL.c_str());
    soup_message_set_request(msg, "application/sdp", SOUP_MEMORY_STATIC, "", 0);

    if (!msg) {
        printf("ERROR: NULL msg in getPostOffer()\n");
        exit(EXIT_FAILURE);
    }
    auto statusCode = soup_session_send_message(session, msg);   

    if (statusCode != 200 && statusCode != 201) {
        printf("(%d):%s\n", statusCode, msg->response_body->data);
        exit(EXIT_FAILURE);
    }

    const char* location = soup_message_headers_get_one(msg->response_headers, "location");
    std::string sdpOffer(msg->response_body->data);

    std::vector<std::string> strVec;
    strVec.push_back(sdpOffer);
    strVec.push_back(location);

    // Cleanup
    g_object_unref(msg);
    g_object_unref(session);

    return strVec;
}

void patchAnswer(CustomData* data)
{

    SoupSession* session = soup_session_new();
    SoupMessage* msg = soup_message_new("PATCH", data->location.c_str());
    if (!msg) {
        printf("ERROR: when creating msg in patchAnswer()\n");
        exit(EXIT_FAILURE);
    }
    const char* sdp = data->sdpAnswer.c_str();

    soup_message_set_request(msg, "application/sdp", SOUP_MEMORY_COPY, sdp, strlen(sdp));
    auto statusCode = soup_session_send_message(session, msg);

    // Cleanup
    g_object_unref(msg);
    g_object_unref(session);

    if (statusCode != 204) {
        printf("(%d):%s\n", statusCode, msg->response_body->data);
        exit(EXIT_FAILURE);
    }
}

int32_t main(int32_t argc, char** argv)
{
    CustomData data;

    if (argc < 2) {
        printf("Usage: GST_PLUGIN_PATH=my/plugin/path/gstreamer-1.0 ./whep-play WHEP-URL\n");
        return 1;
    }

    data.whepURL = argv[1];
    std::vector<std::string> offerLocation = getPostOffer(&data);
    data.sdpOffer = offerLocation[0];
    data.location = offerLocation[1];

    gst_init(nullptr, nullptr);

    // Make elements
    data.webrtc_source = gst_element_factory_make("webrtcbin", "source");
    if (!data.webrtc_source) {
        printf("Failed to make element source. Note: GST_PLUGIN_PATH needs to be set as described in the README.\n");
        return 1;
    }

    data.sinkElement = gst_element_factory_make("glimagesink", "gli_sink");
    if (!data.sinkElement) {
        printf("Failed to make element gli_sink\n");
        return 1;
    }

    data.rtp_depay_vp8 = gst_element_factory_make("rtpvp8depay", "rtp_depayloader_vp8");
    if (!data.rtp_depay_vp8) {
        printf("Failed to make element rtp_depayloader_vp8\n");
        return 1;
    }

    data.vp8_decoder = gst_element_factory_make("vp8dec", "vp8_decoder");
    if (!data.vp8_decoder) {
        printf("Failed to make element vp8_decoder\n");
        return 1;
    }

    data.pipeline = gst_pipeline_new("pipeline");
    if (!data.pipeline) {
        printf("Failed to make element pipeline\n");
        return 1;
    }

    // Add elements
    if (!gst_bin_add(GST_BIN(data.pipeline), data.webrtc_source)) {
        printf("Failed to add element source. Note: GST_PLUGIN_PATH needs to be set as described in the README\n\n");
        return 1;
    }

    if (!gst_bin_add(GST_BIN(data.pipeline), data.rtp_depay_vp8)) {
        printf("Failed to add element rtp_depayloader_vp8\n");
        return 1;
    }

    if (!gst_bin_add(GST_BIN(data.pipeline), data.vp8_decoder)) {
        printf("Failed to add element vp8_decoder\n");
        return 1;
    }

    if (!gst_bin_add(GST_BIN(data.pipeline), data.sinkElement)) {
        printf("Failed to add element gli_sink\n");
        return 1;
    }

    // Signals
    g_signal_connect(data.webrtc_source, "pad-added", G_CALLBACK(padAddedHandler), &data);
    g_signal_connect(data.webrtc_source, "on-negotiation-needed", G_CALLBACK(onNegotiationNeededCallback), &data);

    {
        struct sigaction sigactionData = {};
        sigactionData.sa_handler = intSignalHandler;
        sigactionData.sa_flags = 0;
        sigemptyset(&sigactionData.sa_mask);
        sigaction(SIGINT, &sigactionData, nullptr);
    }

    // Start playing
    printf("Start playing...\n");
    if (gst_element_set_state(data.pipeline, GST_STATE_PLAYING) == GST_STATE_CHANGE_FAILURE) {
        printf("Unable to set the pipeline to the playing state\n");
        return 1;
    }

    printf("Looping...\n");
    mainLoop = g_main_loop_new(nullptr, FALSE);
    g_main_loop_run(mainLoop);

    // Free resources - See CustomData destructor
    g_main_loop_unref(mainLoop);
    gst_element_set_state(data.pipeline, GST_STATE_NULL);
    gst_deinit();
    return 0;
}

void padAddedHandler(GstElement* src, GstPad* new_pad, CustomData* data)
{

    printf("Received new pad '%s' from '%s'\n", GST_PAD_NAME(new_pad), GST_ELEMENT_NAME(src));

    if (!gst_element_link_many(src, data->rtp_depay_vp8, data->vp8_decoder, data->sinkElement, nullptr)) {
        printf("Failed to link source to sink\n");
    }
}

void onNegotiationNeededCallback(GstElement* src, CustomData* data)
{

    handleSDPs(data);
}

void onRemoteDescSetCallback(GstPromise* promise, gpointer userData)
{
    auto data = reinterpret_cast<CustomData*>(userData);

    if (gst_promise_wait(promise) != GST_PROMISE_RESULT_REPLIED) {
        printf("onRemoteDescSetCallback: Failed to receive promise reply\n");
        exit(EXIT_FAILURE);
    }
    gst_promise_unref(promise);

    GstPromise* promiseAnswer = gst_promise_new_with_change_func(onAnswerCreatedCallback, data, nullptr);
    g_signal_emit_by_name(data->webrtc_source, "create-answer", nullptr, promiseAnswer);
}

void onAnswerCreatedCallback(GstPromise* promise, gpointer userData)
{

    auto data = reinterpret_cast<CustomData*>(userData);

    GstWebRTCSessionDescription* answerPointer = nullptr;

    if (gst_promise_wait(promise) != GST_PROMISE_RESULT_REPLIED) {
        printf("onAnswerCreatedCallback: Failed to receive promise reply\n");
        exit(EXIT_FAILURE);
    }
    const GstStructure* reply = gst_promise_get_reply(promise);

    gst_promise_unref(promise);

    gst_structure_get(reply, "answer", GST_TYPE_WEBRTC_SESSION_DESCRIPTION, &answerPointer, nullptr);
    if (!answerPointer->sdp) {
        printf("ERROR: No answer sdp!\n");
    }

    g_signal_emit_by_name(data->webrtc_source, "set-local-description", answerPointer, nullptr);
    data->sdpAnswer = gst_sdp_message_as_text(answerPointer->sdp);
    patchAnswer(data);
}
