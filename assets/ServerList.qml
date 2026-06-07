import bb.cascades 1.4

Container {
    id: serverList

    property string serverName: "lmao server"

    signal channelSelected(string channelName)

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

        onTriggered: {
            var item = channelModel.data(indexPath)

            if (item.type == "channel") {
                serverList.channelSelected(item.name)
            }
        }

        listItemComponents: [
            ListItemComponent {
                type: "category"

                Container {
                    preferredHeight: 2.0
                    horizontalAlignment: HorizontalAlignment.Fill
                    leftPadding: ui.du(2)
                    rightPadding: ui.du(2)

                    layout: DockLayout {}

                    topPadding: ui.du(2.0)
                    Label {
                        text: ListItemData.name.toUpperCase()
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
                            imageSource: "asset:///images/icons/hash.png"

                            preferredWidth: ui.du(4)
                            preferredHeight: ui.du(4)

                            verticalAlignment: VerticalAlignment.Bottom
                            opacity: 0.45

                            scalingMethod: ScalingMethod.AspectFit
                        }

                        Label {
                            text: ListItemData.name
                            textStyle.fontSize: FontSize.Medium
                        }
                    }
                }
            }
        ]

        function itemType(data, indexPath) {
            return data.type
        }
    }

    attachedObjects: [
        ArrayDataModel {
            id: channelModel
        }
    ]

    onCreationCompleted: {
        channelModel.append({"type": "category", "name": "Text Channels"})
        channelModel.append({"type": "channel", "name": "general"})
        channelModel.append({"type": "channel", "name": "random"})
        channelModel.append({"type": "channel", "name": "dev"})

        channelModel.append({"type": "category", "name": "Voice Channels"})
        channelModel.append({"type": "channel", "name": "lounge"})
        channelModel.append({"type": "channel", "name": "music"})
    }
}
