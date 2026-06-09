import bb.cascades 1.4

Container {
    id: serverList

    property string serverName: "lmao server"
    property string serverId: ""

    signal channelSelected(string channelId, string guildId, string channelName)

    horizontalAlignment: HorizontalAlignment.Fill
    verticalAlignment: VerticalAlignment.Fill

    layout: StackLayout {}

    Container {
        preferredHeight: ui.du(10)
        leftPadding: ui.du(2)
        rightPadding: ui.du(2)
        background: Color.create("#111111")
        horizontalAlignment: HorizontalAlignment.Fill

        layout: StackLayout {
            orientation: LayoutOrientation.LeftToRight
        }

        Label {
            text: serverList.serverName
            verticalAlignment: VerticalAlignment.Center
            textStyle.fontSize: FontSize.Medium
            textStyle.fontWeight: FontWeight.Normal
            horizontalAlignment: HorizontalAlignment.Fill
        }
    }

    ListView {
        dataModel: channelModel
        horizontalAlignment: HorizontalAlignment.Fill
        verticalAlignment: VerticalAlignment.Fill

        onTriggered: {
            var item = channelModel.data(indexPath);

            if (item.type == "channel") {
                serverList.channelSelected(item.id, serverList.serverId, item.name);
            }
        }

        listItemComponents: [
            ListItemComponent {
                type: "loading"

                Container {
                    preferredHeight: ui.du(7.0)
                    preferredWidth: ui.du(200)
                    horizontalAlignment: HorizontalAlignment.Fill
                    verticalAlignment: VerticalAlignment.Fill
                    layout: DockLayout {}

                    ActivityIndicator {
                        running: true
                        horizontalAlignment: HorizontalAlignment.Center
                        verticalAlignment: VerticalAlignment.Center
                    }
                }
            },
            ListItemComponent {
                type: "category"

                Container {
                    preferredHeight: 2.0
                    preferredWidth: ui.du(200)
                    horizontalAlignment: HorizontalAlignment.Fill
                    leftPadding: ui.du(2)
                    rightPadding: ui.du(2)

                    layout: DockLayout {}

                    topPadding: ui.du(2.0)
                    Label {
                        text: ListItemData.name.toUpperCase()
                        horizontalAlignment: HorizontalAlignment.Fill
                        verticalAlignment: VerticalAlignment.Center
                        opacity: 0.45
                        textStyle.fontSize: FontSize.XSmall
                    }
                }
            },
            ListItemComponent {
                type: "channel"

                Container {
                    preferredHeight: ui.du(7.0)
                    preferredWidth: ui.du(200)
                    horizontalAlignment: HorizontalAlignment.Fill
                    leftPadding: ui.du(2)
                    rightPadding: ui.du(2)

                    layout: DockLayout {}

                    Container {
                        layout: StackLayout {
                            orientation: LayoutOrientation.LeftToRight
                        }

                        horizontalAlignment: HorizontalAlignment.Fill
                        verticalAlignment: VerticalAlignment.Center

                        ImageView {
                            imageSource: ListItemData.icon

                            preferredWidth: ui.du(4)
                            preferredHeight: ui.du(4)

                            verticalAlignment: VerticalAlignment.Bottom
                            opacity: ListItemData.unread ? 1.0 : 0.45

                            scalingMethod: ScalingMethod.AspectFit
                        }

                        Label {
                            text: ListItemData.name
                            horizontalAlignment: HorizontalAlignment.Fill
                            textStyle.fontSize: FontSize.Medium
                            opacity: ListItemData.unread ? 1.0 : 0.45
                        }
                    }
                }
            }
        ]

        function itemType(data, indexPath) {
            return data.type;
        }

    }

    attachedObjects: [
        ArrayDataModel {
            id: channelModel
        }
    ]

    function refreshChannels() {
        channelModel.clear();
        for (var i = 0; i < appStore.guildChannels.length; ++i) {
            channelModel.append(appStore.guildChannels[i]);
        }
        updateChannelLoading();
    }

    function appendChannels(channels) {
        removeChannelLoading();
        for (var i = 0; i < channels.length; ++i) {
            channelModel.append(channels[i]);
        }
        updateChannelLoading();
    }

    function refreshChannelsIfNeeded() {
        var end = channelModel.size() - loadingRowOffset();
        if (end != appStore.guildChannels.length) {
            refreshChannels();
            return;
        }

        for (var i = 0; i < appStore.guildChannels.length; ++i) {
            var item = channelModel.data([i]);
            if (!item || item.id != appStore.guildChannels[i].id) {
                refreshChannels();
                return;
            }

            if (item.unread != appStore.guildChannels[i].unread) {
                channelModel.replace([i], appStore.guildChannels[i]);
            }
        }
    }

    function loadingRowOffset() {
        if (channelModel.size() == 0) {
            return 0;
        }

        var last = channelModel.data([channelModel.size() - 1]);
        return last.type == "loading" ? 1 : 0;
    }

    function removeChannelLoading() {
        if (channelModel.size() == 0) {
            return;
        }

        var lastIndex = channelModel.size() - 1;
        var last = channelModel.data([lastIndex]);
        if (last.type == "loading") {
            channelModel.removeAt([lastIndex]);
        }
    }

    function updateChannelLoading() {
        removeChannelLoading();
        if (appStore.dataLoading) {
            channelModel.append({
                "type": "loading"
            });
        }
    }

    onCreationCompleted: {
        refreshChannels();
        appStore.guildChannelsAppended.connect(appendChannels);
        appStore.guildChannelsChanged.connect(refreshChannelsIfNeeded);
        appStore.dataLoadingChanged.connect(updateChannelLoading);
    }
}
