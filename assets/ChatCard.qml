import bb.cascades 1.4

Page {
    id: chatPage

    property string channelName: "general"
    property alias title: titleBar.title

    property real viewportHeight: 0
    property real messagesHeight: 0
    property real bottomFillPadding: 0
    property bool pendingScrollToBottom: false
    property string replyAuthor: ""
    property string replyMessage: ""

    signal backRequested()
    signal memberListRequested()

    actionBarVisibility: ChromeVisibility.Hidden

    titleBar: TitleBar {
        id: titleBar
        title: "# " + chatPage.channelName
        visibility: ChromeVisibility.Visible

        dismissAction: ActionItem {
            imageSource: "asset:///images/icons/accent/caret-left.png"

            onTriggered: {
                chatPage.backRequested()
            }
        }

        acceptAction: ActionItem {
            imageSource: "asset:///images/icons/accent/users.png"

            onTriggered: {
                chatPage.memberListRequested()
            }
        }
    }

    Container {
        horizontalAlignment: HorizontalAlignment.Fill
        verticalAlignment: VerticalAlignment.Fill

        layout: StackLayout {}

        Container {
            id: chatArea

            horizontalAlignment: HorizontalAlignment.Fill
            verticalAlignment: VerticalAlignment.Fill

            layout: DockLayout {}

            attachedObjects: [
                LayoutUpdateHandler {
                    onLayoutFrameChanged: {
                        chatPage.viewportHeight = layoutFrame.height
                        chatPage.updateBottomPadding()
                    }
                }
            ]

            ScrollView {
                id: messageScroll

                horizontalAlignment: HorizontalAlignment.Fill
                verticalAlignment: VerticalAlignment.Fill

                scrollViewProperties {
                    scrollMode: ScrollMode.Vertical
                    pinchToZoomEnabled: false
                }

                Container {
                    id: scrollContent

                    horizontalAlignment: HorizontalAlignment.Fill
                    topPadding: chatPage.bottomFillPadding

                    layout: StackLayout {}

                    Container {
                        id: messagesContainer

                        horizontalAlignment: HorizontalAlignment.Fill

                        layout: StackLayout {
                            orientation: LayoutOrientation.TopToBottom
                        }

                        attachedObjects: [
                            LayoutUpdateHandler {
                                onLayoutFrameChanged: {
                                    chatPage.messagesHeight = layoutFrame.height
                                    chatPage.updateBottomPadding()

                                    if (chatPage.pendingScrollToBottom) {
                                        chatPage.scrollToBottomNow()
                                    }
                                }
                            }
                        ]
                    }
                }
            }
        }

        Container {
            horizontalAlignment: HorizontalAlignment.Fill
            background: Color.create("#18191C")

            layout: StackLayout {}

            topPadding: ui.du(1.5)
            bottomPadding: ui.du(1.5)
            leftPadding: ui.du(1.5)
            rightPadding: ui.du(1.5)

            Container {
                visible: chatPage.replyAuthor !== ""
                horizontalAlignment: HorizontalAlignment.Fill
                bottomMargin: ui.du(1.0)

                layout: StackLayout {
                    orientation: LayoutOrientation.LeftToRight
                }

                ImageButton {
                    preferredWidth: ui.du(5.0)
                    preferredHeight: ui.du(5.0)
                    verticalAlignment: VerticalAlignment.Center
                    defaultImageSource: "asset:///images/icons/x.png"

                    onClicked: {
                        chatPage.clearReply()
                    }
                    pressedImageSource: "asset:///images/icons/x-hold.png"
                    disabledImageSource: "asset:///images/icons/x-disabled.png"
                }

                Container {
                    horizontalAlignment: HorizontalAlignment.Fill
                    verticalAlignment: VerticalAlignment.Center

                    Label {
                        text: qsTr("Replying to ") + chatPage.replyAuthor
                        textStyle.fontSize: FontSize.XXSmall
                        textStyle.fontWeight: FontWeight.Bold
                        textStyle.color: Color.create("#F2F3F5")
                    }

                    Label {
                        text: chatPage.replyMessage
                        topMargin: ui.du(-0.3)
                        textStyle.fontSize: FontSize.XXSmall
                        textStyle.color: Color.create("#B5BAC1")
                    }
                }
                
            }

            Container {
                horizontalAlignment: HorizontalAlignment.Fill

                layout: StackLayout {
                    orientation: LayoutOrientation.LeftToRight
                }

                ImageButton {
                    verticalAlignment: VerticalAlignment.Center
                    preferredWidth: ui.du(7.0)
                    preferredHeight: ui.du(7.0)
                    
                    onClicked: {
                        console.log("attach file")
                    }
                    defaultImageSource: "asset:///images/icons/paperclip.png"
                    pressedImageSource: "asset:///images/icons/paperclip-hold.png"
                    disabledImageSource: "asset:///images/icons/paperclip-disabled.png"
                }

                TextArea {
                    id: inputMessage

                    hintText: qsTr("Message #") + chatPage.channelName
                    inputMode: TextAreaInputMode.Text
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
            }
        }
    }

    attachedObjects: [
        ComponentDefinition {
            id: messageBubbleDefinition
            source: "asset:///MessageBubble.qml"
        }
    ]

    function updateBottomPadding() {
        var missingHeight = viewportHeight - messagesHeight

        if (missingHeight > 0) {
            bottomFillPadding = missingHeight
        } else {
            bottomFillPadding = 0
        }
    }

    function appendMessage(message) {
        var bubble = messageBubbleDefinition.createObject()

        bubble.author = message.author
        bubble.initials = message.initials
        bubble.avatarColor = message.avatarColor
        bubble.time = message.time
        bubble.message = message.message
        bubble.replyAuthor = message.replyAuthor || ""
        bubble.replyMessage = message.replyMessage || ""
        bubble.image = message.image
        bubble.imageWidth = message.imageWidth
        bubble.imageHeight = message.imageHeight
        bubble.deleteRequested.connect(function() {
                messagesContainer.remove(bubble)
                bubble.destroy()
                updateBottomPadding()
            })
        bubble.replyRequested.connect(function(author, replyText) {
                chatPage.setReply(author, replyText)
            })

        messagesContainer.add(bubble)

        requestScrollToBottom()
    }

    function requestScrollToBottom() {
        pendingScrollToBottom = true
    }

    function setReply(author, message) {
        replyAuthor = author
        replyMessage = message
        inputMessage.requestFocus()
    }

    function clearReply() {
        replyAuthor = ""
        replyMessage = ""
    }

    function scrollToBottomNow() {
        updateBottomPadding()

        var scrollY = messagesHeight - viewportHeight

        if (scrollY < 0) {
            scrollY = 0
        }

        messageScroll.scrollToPoint(0, scrollY, ScrollAnimation.None)

        pendingScrollToBottom = false
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
            "replyAuthor": replyAuthor,
            "replyMessage": replyMessage,
            "image": "",
            "imageWidth": 0,
            "imageHeight": 0
        })

        inputMessage.text = ""
        clearReply()
    }

    onCreationCompleted: {
        appendMessage({
            "author": "michioxd",
            "initials": "M",
            "avatarColor": "#5865F2",
            "time": "Today 20:10",
            "message": "yo guys",
            "replyAuthor": "",
            "replyMessage": "",
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
            "replyAuthor": "michioxd",
            "replyMessage": "yo guys",
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
            "replyAuthor": "BerryBot",
            "replyMessage": "play some genshit?",
            "image": "",
            "imageWidth": 0,
            "imageHeight": 0
        })
    }
}