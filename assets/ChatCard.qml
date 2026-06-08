import bb.cascades 1.4

Page {
    id: chatPage

    property string channelName: "general"
    property alias title: titleBar.title

    property bool pendingScrollToBottom: false
    property real viewportHeight: 0
    property real messagesHeight: 0
    property real bottomFillPadding: 0
    property int messageIdCounter: 0
    property string replyAuthor: ""
    property string replyMessage: ""

    signal backRequested
    signal memberListRequested

    actionBarVisibility: ChromeVisibility.Hidden

    titleBar: TitleBar {
        id: titleBar
        title: chatPage.channelName
        visibility: ChromeVisibility.Visible

        dismissAction: ActionItem {
            imageSource: "asset:///images/icons/accent/caret-left.png"

            onTriggered: {
                chatPage.backRequested();
            }
        }

        acceptAction: ActionItem {
            imageSource: "asset:///images/icons/accent/users.png"

            onTriggered: {
                chatPage.memberListRequested();
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
                        chatPage.viewportHeight = layoutFrame.height;
                        chatPage.updateBottomPadding();
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
                                    chatPage.messagesHeight = layoutFrame.height;
                                    chatPage.updateBottomPadding();

                                    if (chatPage.pendingScrollToBottom) {
                                        chatPage.scrollToBottomNow();
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
                        chatPage.clearReply();
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
                        console.log("attach file");
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
                            chatPage.sendCurrentMessage();
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
        var missingHeight = viewportHeight - messagesHeight;

        if (missingHeight > 0) {
            bottomFillPadding = missingHeight;
        } else {
            bottomFillPadding = 0;
        }
    }

    function appendMessage(message) {
        var item = normalizeMessage(message);
        var lastIndex = messagesContainer.controls.length - 1;

        if (lastIndex >= 0) {
            var previousBubble = messagesContainer.controls[lastIndex];
            var previous = messageFromBubble(previousBubble);

            if (shouldGroup(previous, item)) {
                previousBubble.isGroupEnd = false;

                item.isGroupStart = false;
                item.showAvatar = false;
                item.showUsername = false;
                item.showTimestamp = false;
            }
        }

        addMessageBubble(item);

        requestScrollToBottom();
    }

    function requestScrollToBottom() {
        pendingScrollToBottom = true;
        scrollToBottomNow();
    }

    function setReply(author, message) {
        replyAuthor = author;
        replyMessage = message;
        inputMessage.requestFocus();
    }

    function clearReply() {
        replyAuthor = "";
        replyMessage = "";
    }

    function addMessageBubble(item) {
        var bubble = messageBubbleDefinition.createObject();

        bubble.author = item.author;
        bubble.initials = item.initials;
        bubble.avatarColor = item.avatarColor;
        bubble.time = item.time;
        bubble.timestampMs = item.timestampMs;
        bubble.message = item.message;
        bubble.replyAuthor = item.replyAuthor;
        bubble.replyMessage = item.replyMessage;
        bubble.image = item.image;
        bubble.imageWidth = item.imageWidth;
        bubble.imageHeight = item.imageHeight;
        bubble.isGroupStart = item.isGroupStart;
        bubble.isGroupEnd = item.isGroupEnd;
        bubble.showAvatar = item.showAvatar;
        bubble.showUsername = item.showUsername;
        bubble.showTimestamp = item.showTimestamp;
        bubble.deleteRequested.connect(function () {
            chatPage.deleteMessageBubble(bubble);
        });
        bubble.replyRequested.connect(function (author, replyText) {
            chatPage.setReply(author, replyText);
        });

        messagesContainer.add(bubble);
    }

    function messageFromBubble(bubble) {
        if (!bubble) {
            return null;
        }

        return {
            "author": bubble.author,
            "timestampMs": bubble.timestampMs
        };
    }

    function normalizeMessage(message) {
        var timestampMs = message.timestampMs || new Date().getTime();

        return {
            "id": ++messageIdCounter,
            "author": message.author || "",
            "initials": message.initials || "",
            "avatarColor": message.avatarColor || "#5865F2",
            "time": message.time || "Now",
            "timestampMs": timestampMs,
            "message": message.message || "",
            "replyAuthor": message.replyAuthor || "",
            "replyMessage": message.replyMessage || "",
            "image": message.image || "",
            "imageWidth": message.imageWidth || 0,
            "imageHeight": message.imageHeight || 0,
            "isGroupStart": true,
            "isGroupEnd": true,
            "showAvatar": true,
            "showUsername": true,
            "showTimestamp": true
        };
    }

    function shouldGroup(previous, current) {
        if (!previous || !current) {
            return false;
        }

        if (previous.author != current.author) {
            return false;
        }

        return current.timestampMs - previous.timestampMs < 7 * 60 * 1000;
    }

    function refreshGroupingAround(index) {
        refreshGroupingAt(index - 1);
        refreshGroupingAt(index);
        refreshGroupingAt(index + 1);
    }

    function refreshGroupingAt(index) {
        if (index < 0 || index >= messagesContainer.controls.length) {
            return;
        }

        var bubble = messagesContainer.controls[index];
        var item = messageFromBubble(bubble);
        var previous = index > 0 ? messageFromBubble(messagesContainer.controls[index - 1]) : null;
        var next = index < messagesContainer.controls.length - 1 ? messageFromBubble(messagesContainer.controls[index + 1]) : null;
        var groupedWithPrevious = shouldGroup(previous, item);
        var groupedWithNext = shouldGroup(item, next);

        bubble.isGroupStart = !groupedWithPrevious;
        bubble.isGroupEnd = !groupedWithNext;
        bubble.showAvatar = bubble.isGroupStart;
        bubble.showUsername = bubble.isGroupStart;
        bubble.showTimestamp = bubble.isGroupStart;
    }

    function deleteMessageBubble(bubble) {
        var index = messagesContainer.controls.indexOf(bubble);

        messagesContainer.remove(bubble);
        bubble.destroy();
        refreshGroupingAround(index);
        updateBottomPadding();
    }

    function scrollToBottomNow() {
        updateBottomPadding();

        var scrollY = messagesHeight - viewportHeight;

        if (scrollY < 0) {
            scrollY = 0;
        }

        messageScroll.scrollToPoint(0, scrollY, ScrollAnimation.None);

        pendingScrollToBottom = false;
    }

    function sendCurrentMessage() {
        var text = inputMessage.text.replace(/^\s+|\s+$/g, "");

        if (text.length == 0) {
            return;
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
            "imageHeight": 0,
            "timestampMs": new Date().getTime()
        });

        inputMessage.text = "";
        clearReply();
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
            "imageHeight": 0,
            "timestampMs": 1000000
        });

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
            "imageHeight": 20,
            "timestampMs": 1000000 + 2 * 60 * 1000
        });

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
            "imageHeight": 0,
            "timestampMs": 1000000 + 5 * 60 * 1000
        });
    }
}
