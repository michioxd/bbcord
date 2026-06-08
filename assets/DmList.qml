import bb.cascades 1.4

Container {
    id: dmList

    signal dmSelected(string channelName)

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
            text: "Direct Messages"
            verticalAlignment: VerticalAlignment.Center
            textStyle.fontSize: FontSize.Medium
            textStyle.fontWeight: FontWeight.Normal
            horizontalAlignment: HorizontalAlignment.Fill
        }
    }

    ListView {
        id: dmListView
        dataModel: dmModel
        verticalAlignment: VerticalAlignment.Fill

        onTriggered: {
            var item = dmModel.data(indexPath);

            if (item.type == "dm") {
                dmList.dmSelected(item.name);
            }
        }

        listItemComponents: [
            ListItemComponent {
                type: "loading"

                Container {
                    preferredHeight: ui.du(7.0)
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
                type: "dm"

                Container {
                    preferredHeight: ui.du(8.8)
                    horizontalAlignment: HorizontalAlignment.Fill
                    leftPadding: ui.du(2)
                    rightPadding: ui.du(2)
                    property string lastAvatarRequestId: ""

                    function requestAvatar() {
                        if (lastAvatarRequestId == ListItemData.id) {
                            return;
                        }
                        if ((ListItemData.avatar === "" && ListItemData.avatarHash !== "") || (ListItemData.avatar2 === "" && ListItemData.avatarHash2 !== "")) {
                            lastAvatarRequestId = ListItemData.id;
                            ListItem.view.loadVisibleDmAvatar(ListItemData.id);
                        }
                    }

                    function safeText(value) {
                        return value === undefined || value === null ? "" : value;
                    }

                    function safeColor(value, fallback) {
                        return value === undefined || value === null || value === "" ? fallback : value;
                    }

                    onCreationCompleted: {
                        requestAvatar();
                    }

                    ListItem.onDataChanged: {
                        if (lastAvatarRequestId != ListItemData.id) {
                            requestAvatar();
                        }
                    }

                    layout: StackLayout {
                        orientation: LayoutOrientation.LeftToRight
                    }

                    Container {
                        preferredWidth: ui.du(7.0)
                        preferredHeight: ui.du(7.0)
                        minWidth: ui.du(7.0)
                        minHeight: ui.du(7.0)
                        maxWidth: ui.du(7.0)
                        maxHeight: ui.du(7.0)
                        verticalAlignment: VerticalAlignment.Center

                        layout: DockLayout {}

                        Container {
                            visible: ListItemData.isGroup != true
                            preferredWidth: ui.du(6.7)
                            preferredHeight: ui.du(6.7)
                            minWidth: ui.du(6.7)
                            minHeight: ui.du(6.7)
                            maxWidth: ui.du(6.7)
                            maxHeight: ui.du(6.7)
                            horizontalAlignment: HorizontalAlignment.Left
                            verticalAlignment: VerticalAlignment.Top
                            background: Color.create(safeColor(ListItemData.avatarColor, "#5865F2"))

                            layout: DockLayout {}

                            ImageView {
                                imageSource: safeText(ListItemData.avatar)
                                visible: ListItemData.avatar !== ""
                                horizontalAlignment: HorizontalAlignment.Fill
                                verticalAlignment: VerticalAlignment.Fill
                                scalingMethod: ScalingMethod.AspectFill
                            }

                            Label {
                                text: ListItemData.initials
                                visible: ListItemData.avatar === ""
                                horizontalAlignment: HorizontalAlignment.Center
                                verticalAlignment: VerticalAlignment.Center
                                textStyle.fontSize: FontSize.Small
                                textStyle.fontWeight: FontWeight.Bold
                                textStyle.color: Color.White
                            }
                        }

                        Container {
                            visible: ListItemData.isGroup == true
                            preferredWidth: ui.du(7.0)
                            preferredHeight: ui.du(7.0)
                            layout: DockLayout {}

                            Container {
                                preferredWidth: ui.du(4.8)
                                preferredHeight: ui.du(4.8)
                                minWidth: ui.du(4.8)
                                minHeight: ui.du(4.8)
                                maxWidth: ui.du(4.8)
                                maxHeight: ui.du(4.8)
                                horizontalAlignment: HorizontalAlignment.Left
                                verticalAlignment: VerticalAlignment.Top
                                background: Color.create(safeColor(ListItemData.avatarColor, "#5865F2"))

                                layout: DockLayout {}

                                ImageView {
                                    imageSource: safeText(ListItemData.avatar)
                                    visible: ListItemData.avatar !== ""
                                    horizontalAlignment: HorizontalAlignment.Fill
                                    verticalAlignment: VerticalAlignment.Fill
                                    scalingMethod: ScalingMethod.AspectFill
                                }

                                Label {
                                    text: ListItemData.initials
                                    visible: ListItemData.avatar === ""
                                    horizontalAlignment: HorizontalAlignment.Center
                                    verticalAlignment: VerticalAlignment.Center
                                    textStyle.fontSize: FontSize.XSmall
                                    textStyle.fontWeight: FontWeight.Bold
                                    textStyle.color: Color.White
                                }
                            }

                            Container {
                                preferredWidth: ui.du(4.8)
                                preferredHeight: ui.du(4.8)
                                minWidth: ui.du(4.8)
                                minHeight: ui.du(4.8)
                                maxWidth: ui.du(4.8)
                                maxHeight: ui.du(4.8)
                                horizontalAlignment: HorizontalAlignment.Right
                                verticalAlignment: VerticalAlignment.Bottom
                                background: Color.create(safeColor(ListItemData.avatarColor2, "#3F4147"))

                                layout: DockLayout {}

                                ImageView {
                                    imageSource: safeText(ListItemData.avatar2)
                                    visible: ListItemData.avatar2 !== ""
                                    horizontalAlignment: HorizontalAlignment.Fill
                                    verticalAlignment: VerticalAlignment.Fill
                                    scalingMethod: ScalingMethod.AspectFill
                                }

                                Label {
                                    text: ListItemData.initials2
                                    visible: ListItemData.avatar2 === ""
                                    horizontalAlignment: HorizontalAlignment.Center
                                    verticalAlignment: VerticalAlignment.Center
                                    textStyle.fontSize: FontSize.XSmall
                                    textStyle.fontWeight: FontWeight.Bold
                                    textStyle.color: Color.White
                                }
                            }
                        }

                        Container {
                            preferredWidth: ui.du(1.5)
                            preferredHeight: ui.du(1.5)
                            minWidth: ui.du(1.5)
                            minHeight: ui.du(1.5)
                            maxWidth: ui.du(1.5)
                            maxHeight: ui.du(1.5)
                            horizontalAlignment: HorizontalAlignment.Right
                            verticalAlignment: VerticalAlignment.Bottom
                            background: Color.create(safeColor(ListItemData.statusColor, "#80848E"))
                        }
                    }

                    Label {
                        text: ListItemData.name
                        leftMargin: ui.du(1.5)
                        verticalAlignment: VerticalAlignment.Center
                        textStyle.fontSize: FontSize.Medium
                    }
                }
            }
        ]

        function itemType(data, indexPath) {
            return data.type;
        }

        function loadVisibleDmAvatar(channelId) {
            dmListController.loadDmAvatar(channelId);
        }

        attachedObjects: [
            ListScrollStateHandler {
                onAtEndChanged: {
                    if (atEnd) {
                        dmListController.loadMoreDmChannels();
                    }
                }
            }
        ]
    }

    attachedObjects: [
        ArrayDataModel {
            id: dmModel
        }
    ]

    function refreshDms() {
        dmModel.clear();
        for (var i = 0; i < appStore.dmChannels.length; ++i) {
            dmModel.append(appStore.dmChannels[i]);
        }
        updateDmLoading();
    }

    function appendDms(channels) {
        removeDmLoading();
        for (var i = 0; i < channels.length; ++i) {
            dmModel.append(channels[i]);
        }
        updateDmLoading();
    }

    function refreshDmsIfNeeded() {
        var end = dmModel.size() - loadingRowOffset();
        if (end != appStore.dmChannels.length) {
            refreshDms();
            return;
        }

        for (var i = 0; i < appStore.dmChannels.length; ++i) {
            var item = dmModel.data([i]);
            if (!item || item.id != appStore.dmChannels[i].id) {
                refreshDms();
                return;
            }

            if (item.name != appStore.dmChannels[i].name || item.avatar != appStore.dmChannels[i].avatar || item.avatar2 != appStore.dmChannels[i].avatar2 || item.statusColor != appStore.dmChannels[i].statusColor) {
                dmModel.replace([i], appStore.dmChannels[i]);
            }
        }
    }

    function loadingRowOffset() {
        if (dmModel.size() == 0) {
            return 0;
        }

        var last = dmModel.data([dmModel.size() - 1]);
        return last.type == "loading" ? 1 : 0;
    }

    function removeDmLoading() {
        if (dmModel.size() == 0) {
            return;
        }

        var lastIndex = dmModel.size() - 1;
        var last = dmModel.data([lastIndex]);
        if (last.type == "loading") {
            dmModel.removeAt([lastIndex]);
        }
    }

    function updateDmLoading() {
        removeDmLoading();
        if (appStore.dataLoading) {
            dmModel.append({
                "type": "loading"
            });
        }
    }

    onCreationCompleted: {
        refreshDms();
        appStore.dmChannelsAppended.connect(appendDms);
        appStore.dmAvatarChanged.connect(updateDmAvatar);
        appStore.dmAvatar2Changed.connect(updateDmAvatar2);
        appStore.dmChannelsChanged.connect(refreshDmsIfNeeded);
        appStore.dataLoadingChanged.connect(updateDmLoading);
    }

    function updateDmAvatar(channelId, avatarSource) {
        if (dmModel.size() == 0 || !avatarSource)
            return;
        var end = dmModel.size() - loadingRowOffset();
        for (var i = 0; i < end; ++i) {
            var item = dmModel.data([i]);
            if (item && item.id == channelId) {
                item.avatar = avatarSource;
                dmModel.replace([i], item);
                break;
            }
        }
    }

    function updateDmAvatar2(channelId, avatarSource) {
        if (dmModel.size() == 0 || !avatarSource)
            return;
        var end = dmModel.size() - loadingRowOffset();
        for (var i = 0; i < end; ++i) {
            var item = dmModel.data([i]);
            if (item && item.id == channelId) {
                item.avatar2 = avatarSource;
                dmModel.replace([i], item);
                break;
            }
        }
    }
}
