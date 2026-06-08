import bb.cascades 1.4
import bb.system 1.2

Page {
    id: chatPage

    property string channelName: "general"
    property alias title: titleBar.title

    property bool pendingScrollToBottom: false
    property bool suppressOlderLoadUntilBottom: false
    property real viewportHeight: 0
    property real messagesHeight: 0
    property real bottomFillPadding: 0
    property int maxRenderedMessages: 25
    property int maxAutoImagePreviews: 0
    property bool bulkRefreshingMessages: false
    property string renderedChannelId: ""
    property variant avatarSourceCache: ({})
    property variant requestedAvatarIds: ({})
    property variant failedAttachmentImages: ({})
    property string replyMessageId: ""
    property string replyAuthor: ""
    property string replyMessage: ""
    property string editingMessageId: ""
    property bool active: true

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
                chatPage.deactivatePage();
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

                onViewableAreaChanged: {
                    chatPage.requestOlderIfNearTop();
                }

                Container {
                    id: scrollContent
                    horizontalAlignment: HorizontalAlignment.Fill
                    topPadding: chatPage.bottomFillPadding

                    layout: StackLayout {}

                    Container {
                        visible: appStore.chatLoadingBefore
                        preferredHeight: ui.du(7.0)
                        horizontalAlignment: HorizontalAlignment.Fill

                        layout: DockLayout {}

                        ActivityIndicator {
                            running: appStore.chatLoadingBefore
                            horizontalAlignment: HorizontalAlignment.Center
                            verticalAlignment: VerticalAlignment.Center
                        }
                    }

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
                visible: chatController.hasPendingAttachment
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
                    pressedImageSource: "asset:///images/icons/x-hold.png"
                    disabledImageSource: "asset:///images/icons/x-disabled.png"

                    onClicked: {
                        chatController.clearPendingAttachment();
                    }
                }

                Container {
                    visible: chatController.pendingAttachmentIsImage
                    preferredWidth: ui.du(8.0)
                    preferredHeight: ui.du(8.0)
                    leftMargin: ui.du(1.0)

                    ImageView {
                        imageSource: chatController.pendingAttachmentPreview
                        horizontalAlignment: HorizontalAlignment.Fill
                        verticalAlignment: VerticalAlignment.Fill
                        scalingMethod: ScalingMethod.AspectFit
                    }
                }

                Container {
                    horizontalAlignment: HorizontalAlignment.Fill
                    verticalAlignment: VerticalAlignment.Center
                    leftMargin: ui.du(1.0)

                    Label {
                        text: qsTr("Attachment")
                        textStyle.fontSize: FontSize.XXSmall
                        textStyle.fontWeight: FontWeight.Bold
                        textStyle.color: Color.create("#F2F3F5")
                    }

                    Label {
                        text: chatController.pendingAttachmentName
                        topMargin: ui.du(-0.3)
                        textStyle.fontSize: FontSize.XXSmall
                        textStyle.color: Color.create("#B5BAC1")
                    }
                }
            }

            Label {
                visible: chatController.pendingAttachmentError !== ""
                text: chatController.pendingAttachmentError
                bottomMargin: ui.du(1.0)
                textStyle.fontSize: FontSize.XXSmall
                textStyle.color: Color.create("#ED4245")
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
                        attachmentPathPrompt.show();
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

            Container {
                visible: chatPage.editingMessageId !== ""
                horizontalAlignment: HorizontalAlignment.Fill
                topMargin: ui.du(0.8)

                layout: StackLayout {
                    orientation: LayoutOrientation.LeftToRight
                }

                Label {
                    text: qsTr("Editing message")
                    horizontalAlignment: HorizontalAlignment.Fill
                    textStyle.fontSize: FontSize.XXSmall
                    textStyle.color: Color.create("#B5BAC1")
                }

                ImageButton {
                    preferredWidth: ui.du(5.0)
                    preferredHeight: ui.du(5.0)
                    defaultImageSource: "asset:///images/icons/x.png"
                    pressedImageSource: "asset:///images/icons/x-hold.png"
                    disabledImageSource: "asset:///images/icons/x-disabled.png"

                    onClicked: {
                        chatPage.cancelEdit();
                    }
                }
            }
        }
    }

    attachedObjects: [
        ComponentDefinition {
            id: messageBubbleDefinition
            source: "asset:///MessageBubble.qml"
        },
        SystemPrompt {
            id: attachmentPathPrompt
            title: qsTr("Attach file")
            modality: SystemUiModality.Application
            inputField.inputMode: SystemUiInputMode.Default
            inputField.emptyText: qsTr("/accounts/1000/shared/...")
            confirmButton.label: qsTr("Attach")
            cancelButton.label: qsTr("Cancel")

            onFinished: {
                if (result == SystemUiResult.ConfirmButtonSelection) {
                    chatController.setPendingAttachment(inputFieldTextEntry());
                }
            }
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

    function refreshMessages(scrollToBottom) {
        if (!active) {
            return;
        }

        renderedChannelId = chatController.currentChannelId;
        clearMessageBubbles();

        var messages = chatController.currentMessages();
        var start = messages.length - maxRenderedMessages;
        if (start < 0) {
            start = 0;
        }

        bulkRefreshingMessages = true;
        for (var i = start; i < messages.length; ++i) {
            appendMessage(messages[i], false);
        }
        bulkRefreshingMessages = false;

        if (scrollToBottom) {
            requestScrollToBottom();
        } else {
            updateBottomPadding();
        }
    }

    function clearMessageBubbles() {                                      
        while (messagesContainer.controls.length > 0) {
            var bubble = messagesContainer.controls[0];
            cancelBubbleImageRequest(bubble);
            messagesContainer.remove(bubble);
            bubble.destroy();
        }
    }

    function appendMessage(message, scrollToBottom) {
        if (!active) {
            return;
        }

        if (message.channelId && message.channelId != renderedChannelId) {
            return;
        }

        var wasNearBottom = isNearBottom();
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

        trimRenderedMessagesFromTop();
        if (messagesContainer.controls.length >= maxRenderedMessages) {
            regroupVisibleMessages();
        }

        if (scrollToBottom || wasNearBottom || item.pending) {
            requestScrollToBottom();
        } else {
            updateBottomPadding();
        }
    }

    function prependMessages(messages) {
        if (!active) {
            return;
        }

        if (chatController.currentChannelId != renderedChannelId) {
            return;
        }

        var previousHeight = messagesHeight;
        var normalized = [];

        for (var i = 0; i < messages.length; ++i) {
            normalized.push(normalizeMessage(messages[i]));
        }

        for (var j = normalized.length - 1; j >= 0; --j) {
            addMessageBubbleAtTop(normalized[j]);
        }

        regroupVisibleMessages();
        updateBottomPadding();
        messageScroll.scrollToPoint(0, messageScroll.viewableArea.y + messagesHeight - previousHeight, ScrollAnimation.None);
    }

    function requestScrollToBottom() {
        suppressOlderLoadUntilBottom = true;
        pendingScrollToBottom = true;
    }

    function isNearBottom() {
        var scrollY = messageScroll.viewableArea.y;
        var visibleBottom = scrollY + viewportHeight;
        var threshold = ui.du(4.0);

        return messagesHeight <= viewportHeight || visibleBottom + threshold >= messagesHeight;
    }

    function setReply(messageId, author, message) {
        replyMessageId = messageId;
        replyAuthor = author;
        replyMessage = message;
        cancelEdit();
        inputMessage.requestFocus();
    }

    function clearReply() {
        replyMessageId = "";
        replyAuthor = "";
        replyMessage = "";
    }

    function startEdit(messageId, message) {
        editingMessageId = messageId;
        inputMessage.text = message;
        clearReply();
        inputMessage.requestFocus();
    }

    function cancelEdit() {
        editingMessageId = "";
    }

    function addMessageBubble(item) {
        var bubble = messageBubbleDefinition.createObject();

        setupMessageBubble(bubble, item);
        messagesContainer.add(bubble);
    }

    function addMessageBubbleAtTop(item) {
        var bubble = messageBubbleDefinition.createObject();

        setupMessageBubble(bubble, item);
        messagesContainer.insert(0, bubble);
    }

    function setupMessageBubble(bubble, item) {
        bubble.messageId = item.id;
        bubble.authorId = item.authorId;
        bubble.author = item.author;
        bubble.initials = item.initials;
        bubble.avatarSource = item.avatarSource;
        bubble.avatarColor = item.avatarColor;
        bubble.time = item.time;
        bubble.timestampMs = item.timestampMs;
        bubble.message = item.message;
        bubble.replyAuthor = item.replyAuthor;
        bubble.replyMessage = item.replyMessage;
        bubble.image = item.image;
        bubble.imageLoadFailed = item.imageLoadFailed;
        bubble.imageWidth = item.imageWidth;
        bubble.imageHeight = item.imageHeight;
        bubble.attachmentUrl = item.attachmentUrl;
        bubble.attachmentName = item.attachmentName;
        bubble.attachmentIsImage = item.attachmentIsImage;
        bubble.isGroupStart = item.isGroupStart;
        bubble.isGroupEnd = item.isGroupEnd;
        bubble.showAvatar = item.showAvatar;
        bubble.showUsername = item.showUsername;
        bubble.showTimestamp = item.showTimestamp;
        bubble.pending = item.pending;
        bubble.failed = item.failed;
        bubble.edited = item.edited;
        bubble.deleteRequested.connect(function (messageId) {
            chatController.deleteMessage(messageId);
        });
        bubble.editRequested.connect(function (messageId, messageText) {
            chatPage.startEdit(messageId, messageText);
        });
        bubble.replyRequested.connect(function (messageId, author, replyText) {
            chatPage.setReply(messageId, author, replyText);
        });
        bubble.attachmentOpenRequested.connect(function (url) {
            chatController.openAttachment(url);
        });
        bubble.attachmentImageLoadRequested.connect(function (url) {
            chatPage.loadAttachmentImage(url);
        });

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
        var authorId = message.authorId || "";
        var avatarHash = message.avatarHash || "";
        var avatarSource = "";
        var attachmentUrl = message.attachmentUrl || "";
        var attachmentIsImage = message.attachmentIsImage == true;
        var imageSource = message.image || "";
        var imageLoadFailed = attachmentIsImage && attachmentUrl !== "";

        if (isRemoteImageSource(imageSource)) {
            if (attachmentUrl === "") {
                attachmentUrl = imageSource;
            }
            imageSource = "";
        }

        if (attachmentIsImage) {
            imageSource = "";
            imageLoadFailed = true;
        }

        if (authorId !== "") {
            avatarSource = avatarSourceCache[authorId] || "";
            if (avatarSource === "" && avatarHash !== "" && requestedAvatarIds[authorId] !== true) {
                requestedAvatarIds[authorId] = true;
                avatarSource = chatController.avatarSource(authorId, avatarHash);
                if (avatarSource !== "") {
                    avatarSourceCache[authorId] = avatarSource;
                }
            }
        }

        return {
            "id": message.id || "",
            "author": message.author || "",
            "authorId": authorId,
            "initials": message.initials || "",
            "avatarHash": avatarHash,
            "avatarSource": avatarSource,
            "avatarColor": message.avatarColor || "#5865F2",
            "time": message.time || "Now",
            "timestampMs": timestampMs,
            "message": message.message || "",
            "replyAuthor": message.replyAuthor || "",
            "replyMessage": message.replyMessage || "",
            "image": imageSource,
            "imageWidth": message.imageWidth || 0,
            "imageHeight": message.imageHeight || 0,
            "attachmentUrl": attachmentUrl,
            "attachmentName": message.attachmentName || "",
            "attachmentIsImage": attachmentIsImage,
            "imageLoadFailed": imageLoadFailed,
            "pending": message.pending == true,
            "failed": message.failed == true,
            "edited": message.edited == true,
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

    function isRemoteImageSource(source) {
        if (!source) {
            return false;
        }

        var lower = source.toLowerCase();
        return lower.indexOf("https://") == 0 || lower.indexOf("http://") == 0;
    }

    function regroupVisibleMessages() {
        var previous = null;

        for (var i = 0; i < messagesContainer.controls.length; ++i) {
            var bubble = messagesContainer.controls[i];
            var current = messageFromBubble(bubble);
            var grouped = shouldGroup(previous, current);

            bubble.isGroupStart = !grouped;
            bubble.showAvatar = !grouped;
            bubble.showUsername = !grouped;
            bubble.showTimestamp = !grouped;
            bubble.isGroupEnd = true;

            if (grouped && i > 0) {
                messagesContainer.controls[i - 1].isGroupEnd = false;
            }

            previous = current;
        }
    }

    function trimRenderedMessagesFromTop() {
        var removeCount = messagesContainer.controls.length - maxRenderedMessages;

        while (removeCount > 0 && messagesContainer.controls.length > 0) {
            var bubble = messagesContainer.controls[0];
            cancelBubbleImageRequest(bubble);
            messagesContainer.remove(bubble);
            bubble.destroy();
            --removeCount;
        }
    }

    function scrollToBottomNow() {
        updateBottomPadding();

        if (viewportHeight <= 0 || messagesHeight <= 0) {
            return;
        }

        var scrollY = messagesHeight - viewportHeight;

        if (scrollY < 0) {
            scrollY = 0;
        }

        messageScroll.scrollToPoint(0, scrollY, ScrollAnimation.None);

        pendingScrollToBottom = false;
        suppressOlderLoadUntilBottom = false;
    }

    function sendCurrentMessage() {
        var text = inputMessage.text.replace(/^\s+|\s+$/g, "");

        if (text.length == 0 && !chatController.hasPendingAttachment) {
            return;
        }

        if (editingMessageId !== "") {
            if (chatController.hasPendingAttachment) {
                chatController.clearPendingAttachment();
            }
            chatController.editMessage(editingMessageId, text);
            cancelEdit();
        } else {
            chatController.sendMessage(text, replyMessageId, replyAuthor, replyMessage);
            clearReply();
        }

        inputMessage.text = "";
    }

    function requestOlderIfNearTop() {
        if (!active) {
            return;
        }

        if (pendingScrollToBottom || suppressOlderLoadUntilBottom) {
            return;
        }

        if (messagesContainer.controls.length <= 0) {
            return;
        }

        if (messageScroll.viewableArea.y > ui.du(2.0)) {
            return;
        }

        if (chatController.isLoadingBefore() || !chatController.hasMoreBefore()) {
            return;
        }

        chatController.requestOlderMessages();
    }

    function clearForChannelSwitch() {
        if (!active) {
            return;
        }

        renderedChannelId = chatController.currentChannelId;
        pendingScrollToBottom = false;
        suppressOlderLoadUntilBottom = true;
        replyMessageId = "";
        replyAuthor = "";
        replyMessage = "";
        editingMessageId = "";
        clearMessageBubbles();
        bottomFillPadding = 0;
        messageScroll.scrollToPoint(0, 0, ScrollAnimation.None);
    }

    function deactivatePage() {
        active = false;
        clearMessageBubbles();
    }

    function reactivatePage() {
        if (active) {
            return;
        }

        active = true;
        refreshMessages(true);
    }

    onCreationCompleted: {
        active = true;
        renderedChannelId = chatController.currentChannelId;
        refreshMessages(true);
        chatController.currentChannelChanged.connect(function () {
            chatPage.channelName = chatController.currentChannelName;
            chatPage.clearForChannelSwitch();
            if (chatController.isInitialLoaded()) {
                chatPage.refreshMessages(true);
            }
        });
        appStore.chatMessagesReset.connect(function (channelId, messages) {
            if (channelId == chatPage.renderedChannelId) {
                chatPage.refreshMessages(true);
            }
        });
        appStore.chatMessagesPrepended.connect(function (channelId, messages) {
            if (channelId == chatPage.renderedChannelId) {
                chatPage.prependMessages(messages);
            }
        });
        appStore.chatMessageAdded.connect(function (channelId, message) {
            if (channelId == chatPage.renderedChannelId) {
                chatPage.appendMessage(message, false);
            }
        });
        appStore.chatMessageUpdated.connect(function (channelId, message) {
            if (channelId == chatPage.renderedChannelId) {
                chatPage.refreshMessages(false);
            }
        });
        appStore.chatMessageDeleted.connect(function (channelId, messageId) {
            if (channelId == chatPage.renderedChannelId) {
                chatPage.refreshMessages(false);
            }
        });
        appStore.chatAvatarChanged.connect(function (userId, avatarSource) {
            chatPage.updateAvatarSource(userId, avatarSource);
        });
        chatController.attachmentImageCached.connect(function (url, imageSource) {
            chatPage.updateCachedAttachmentImage(url, imageSource);
        });
        chatController.attachmentImageFailed.connect(function (url) {
            chatPage.markAttachmentImageFailed(url);
        });
    }

    function updateAvatarSource(userId, avatarSource) {
        if (userId === "" || avatarSource === "") {
            return;
        }

        for (var i = 0; i < messagesContainer.controls.length; ++i) {
            var bubble = messagesContainer.controls[i];
            if (bubble.authorId == userId) {
                bubble.avatarSource = avatarSource;
            }
        }

        avatarSourceCache[userId] = avatarSource;
        requestedAvatarIds[userId] = true;
    }

    function loadAttachmentImage(url) {
        if (url === "") {
            return;
        }

        var imageSource = "";
        try {
            imageSource = chatController.cachedImageSource(url);
        } catch (e) {
            imageSource = "";
        }

        if (imageSource !== "" && !isRemoteImageSource(imageSource)) {
            updateCachedAttachmentImage(url, imageSource);
            return;
        }

        try {
            chatController.requestCachedImage(url);
        } catch (e2) {
            markAttachmentImageFailed(url);
        }
    }

    function updateCachedAttachmentImage(url, imageSource) {
        if (url === "" || imageSource === "") {
            return;
        }

        for (var i = 0; i < messagesContainer.controls.length; ++i) {
            var bubble = messagesContainer.controls[i];
            if (bubble.attachmentUrl == url) {
                bubble.imageLoadFailed = false;
                bubble.image = imageSource;
            }
        }
    }

    function requestVisibleImagePreviews() {
        return;

        var requested = 0;

        for (var i = messagesContainer.controls.length - 1; i >= 0 && requested < maxAutoImagePreviews; --i) {
            var bubble = messagesContainer.controls[i];
            if (bubble.attachmentIsImage && bubble.attachmentUrl !== "" && bubble.image === "" && !bubble.imageLoadFailed) {
                try {
                    chatController.requestCachedImage(bubble.attachmentUrl);
                    ++requested;
                } catch (e) {
                    bubble.imageLoadFailed = true;
                    failedAttachmentImages[bubble.attachmentUrl] = true;
                }
            }
        }
    }

    function markAttachmentImageFailed(url) {
        if (url === "") {
            return;
        }

        failedAttachmentImages[url] = true;
        for (var i = 0; i < messagesContainer.controls.length; ++i) {
            var bubble = messagesContainer.controls[i];
            if (bubble.attachmentUrl == url) {
                bubble.image = "";
                bubble.imageLoadFailed = true;
            }
        }
    }

    function cancelBubbleImageRequest(bubble) {
        if (bubble && bubble.attachmentIsImage && bubble.attachmentUrl !== "" && bubble.image === "") {
            chatController.cancelCachedImage(bubble.attachmentUrl);
        }
    }
}
