FROM gitpod/workspace-full-vnc

ENV TRIGGER_REBUILD=4

ENV PATH=${PATH}:/home/gitpod/depot_tools

RUN cd ~ && git clone https://chromium.googlesource.com/chromium/tools/depot_tools.git --depth=1

RUN sudo apt update && sudo apt install -y libgtk-3-dev libnotify-dev libwebkit2gtk-4.0-dev -y