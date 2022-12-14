[![Slack](http://slack.streamingtech.se/badge.svg)](http://slack.streamingtech.se)

# Player for WebRTC HTTP Egress Protocol

Video player for WebRTC streams using WHEP as SDP exchange protocol

## Build from source

### OSX

Requirements:
- XCode command line tools installed
- Install additional dependencies using homebrew

```
brew install gstreamer gst-plugins-bad libsoup@2 icu4c cmake
```

```
cmake -DCMAKE_BUILD_TYPE=Release -G "Unix Makefiles" .
make
sudo make install
```

## Run

To run you need to set the `GST_PLUGIN_PATH` environment variable to where you have the gstreamer plugins installed, e.g:

```
export GST_PLUGIN_PATH=/opt/homebrew/lib/gstreamer-1.0
```

Then run the command.
```
whep-play <WHEP URL>
```

## Debug

Run with the following environment variables set:

```
GST_DEBUG_DUMP_DOT_DIR=<target-dir> \
GST_DEBUG=4 \
GST_PLUGIN_PATH=/opt/homebrew/lib/gstreamer-1.0 \
./whep-play <WHEP URL>
```

Generate png from .dot
```
dot -Tpng input.dot > output.png
```

## License (Apache-2.0)

```
Copyright 2022 Eyevinn Technology AB

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
```

## Support

Join our [community on Slack](http://slack.streamingtech.se) where you can post any questions regarding any of our open source projects. Eyevinn's consulting business can also offer you:

- Further development of this component
- Customization and integration of this component into your platform
- Support and maintenance agreement

Contact [sales@eyevinn.se](mailto:sales@eyevinn.se) if you are interested.

## About Eyevinn Technology

[Eyevinn Technology](https://www.eyevinntechnology.se) is an independent consultant firm specialized in video and streaming. Independent in a way that we are not commercially tied to any platform or technology vendor. As our way to innovate and push the industry forward we develop proof-of-concepts and tools. The things we learn and the code we write we share with the industry in [blogs](https://dev.to/video) and by open sourcing the code we have written.

Want to know more about Eyevinn and how it is to work here. Contact us at work@eyevinn.se!
