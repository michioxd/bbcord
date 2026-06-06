import bb.cascades 1.4

Page {
    id: chatPage
    property string channelName: "general"
    property alias title: titleBar.title
    signal backRequested()
    actionBarVisibility: ChromeVisibility.Hidden

    titleBar: TitleBar {
        id: titleBar
        title: "# " + chatPage.channelName
        visibility: ChromeVisibility.Visible

        dismissAction: ActionItem {
            title: qsTr("Back")

            onTriggered: {
                chatPage.backRequested()
            }
        }
    }

    Container {
        horizontalAlignment: HorizontalAlignment.Fill
        verticalAlignment: VerticalAlignment.Fill

        layout: StackLayout {}

        ListView {
            id: messageList
            dataModel: messageModel

            horizontalAlignment: HorizontalAlignment.Fill
            verticalAlignment: VerticalAlignment.Fill

            listItemComponents: [
                ListItemComponent {
                    type: "message"

                    Container {
                        horizontalAlignment: HorizontalAlignment.Fill
                        leftPadding: ui.du(2.0)
                        rightPadding: ui.du(2.0)
                        topPadding: ui.du(1.5)
                        bottomPadding: ui.du(1.0)

                        layout: StackLayout {
                            orientation: LayoutOrientation.LeftToRight
                        }

                        Container {
                            preferredWidth: ui.du(8.0)
                            verticalAlignment: VerticalAlignment.Top

                            Container {
                                preferredWidth: ui.du(7.0)
                                preferredHeight: ui.du(7.0)
                                maxWidth: ui.du(7.0)
                                minWidth: ui.du(7.0)
                                maxHeight: ui.du(7.0)
                                minHeight: ui.du(7.0)
                                background: Color.create(ListItemData.avatarColor)

                                layout: DockLayout {}

                                Label {
                                    text: ListItemData.initials
                                    horizontalAlignment: HorizontalAlignment.Center
                                    verticalAlignment: VerticalAlignment.Center
                                    textStyle.fontSize: FontSize.Small
                                    textStyle.fontWeight: FontWeight.Bold
                                    textStyle.color: Color.White
                                }
                            }
                        }

                        Container {
                            horizontalAlignment: HorizontalAlignment.Fill
                            leftMargin: ui.du(1.5)

                            Container {
                                horizontalAlignment: HorizontalAlignment.Fill

                                layout: StackLayout {
                                    orientation: LayoutOrientation.LeftToRight
                                }

                                Label {
                                    text: ListItemData.author
                                    textStyle.fontSize: FontSize.Small
                                    textStyle.fontWeight: FontWeight.Bold
                                    textStyle.color: Color.create("#F2F3F5")
                                    verticalAlignment: VerticalAlignment.Center
                                }

                                Label {
                                    text: ListItemData.time
                                    leftMargin: ui.du(1.0)
                                    opacity: 0.5
                                    textStyle.fontSize: FontSize.XSmall
                                    textStyle.color: Color.create("#C9CDD3")
                                    verticalAlignment: VerticalAlignment.Center
                                }
                            }

                            Label {
                                text: ListItemData.message
                                multiline: true
                                textStyle.fontSize: FontSize.Small
                                textStyle.color: Color.create("#DCDDDE")
                            }

                            Container {
                                visible: ListItemData.image !== ""
                                preferredWidth: ui.du(ListItemData.imageWidth)
                                preferredHeight: ui.du(ListItemData.imageHeight)
                                topMargin: ui.du(1.0)

                                layout: DockLayout {}

                                ImageView {
                                    imageSource: ListItemData.image
                                    horizontalAlignment: HorizontalAlignment.Fill
                                    verticalAlignment: VerticalAlignment.Fill
                                    scalingMethod: ScalingMethod.AspectFill
                                }
                            }
                        }
                    }
                }
            ]

            function itemType(data, indexPath) {
                return "message"
            }

            function scrollToLatestMessage() {
                if (messageModel.size() > 0) {
                    scrollToItem([ messageModel.size() - 1 ], ScrollAnimation.None)
                }
            }
        }

        Container {
            horizontalAlignment: HorizontalAlignment.Fill
            preferredHeight: ui.du(11.0)
            leftPadding: ui.du(1.5)
            rightPadding: ui.du(1.5)
            topPadding: ui.du(1.2)
            bottomPadding: ui.du(1.2)
            background: Color.create("#18191C")

            layout: StackLayout {
                orientation: LayoutOrientation.LeftToRight
            }

            TextField {
                id: inputMessage
                hintText: qsTr("Message #") + chatPage.channelName
                inputMode: TextFieldInputMode.Text
                textFormat: TextFormat.Plain
                horizontalAlignment: HorizontalAlignment.Fill
                verticalAlignment: VerticalAlignment.Center

                input {
                    submitKey: SubmitKey.Send

                    onSubmitted: {
                        chatPage.sendCurrentMessage()
                    }
                }
            }

            Button {
                imageSource: "asset:///images/icons/paperclip.png"
                preferredWidth: ui.du(8.0)
                leftMargin: ui.du(1.0)
                verticalAlignment: VerticalAlignment.Center

                onClicked: {
                    console.log("attach file")
                }
            }
        }
    }

    attachedObjects: [
        ArrayDataModel {
            id: messageModel
        }
    ]

    function appendMessage(message) {
        messageModel.append(message)
        messageList.scrollToLatestMessage()
    }

    function sendCurrentMessage() {
        var text = inputMessage.text.replace(/^\s+|\s+$/g, "")

        if (text.length == 0) {
            return
        }

        appendMessage({
                "author": "You",
                "initials": "Y",
                "avatarColor": "#5865F2",
                "time": "Now",
                "message": text,
                "image": "",
                "imageWidth": 0,
                "imageHeight": 0
            })

        inputMessage.text = ""
    }

    onCreationCompleted: {
        appendMessage({
                "author": "michioxd",
                "initials": "M",
                "avatarColor": "#5865F2",
                "time": "Today 20:10",
                "message": "yo guys",
                "image": "",
                "imageWidth": 0,
                "imageHeight": 0
            })
            appendMessage({
                "author": "BerryBot",
                "initials": "B",
                "avatarColor": "#43B581",
                "time": "Today 20:12",
                "message": "play some genshit?",
                "image": "asset:///images/demo.png",
                "imageWidth": 32,
                "imageHeight": 20
            })
            appendMessage({
                "author": "Guest",
                "initials": "G",
                "avatarColor": "#FAA61A",
                "time": "Today 20:15",
                "message": "hell nah nooooo...",
                "image": "",
                "imageWidth": 0,
                "imageHeight": 0
            })
    }
}
