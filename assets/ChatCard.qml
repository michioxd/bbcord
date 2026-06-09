import bb.cascades 1.4
import bb.system 1.2

Page {
    id: chatPage

    property string channelName: "general"
    property alias title: titleBar.title

    property string replyMessageId: ""
    property string replyAuthor: ""
    property string replyMessage: ""
    property string editingMessageId: ""
    property bool active: true
    property bool olderLoadRequested: false

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

            ListView {
                id: messageList
                horizontalAlignment: HorizontalAlignment.Fill
                verticalAlignment: VerticalAlignment.Fill
                dataModel: chatController.chatDataModel
                stickToEdgePolicy: ListViewStickToEdgePolicy.End
                scrollRole: ScrollRole.Main

                listItemComponents: [
                    ListItemComponent {
                        type: ""

                        MessageBubble {
                            messageId: ListItemData.id
                            authorId: ListItemData.authorId
                            author: ListItemData.author
                            initials: ListItemData.initials
                            avatarSource: ListItemData.avatarSource
                            avatarColor: ListItemData.avatarColor
                            time: ListItemData.time
                            timestampMs: ListItemData.timestampMs
                            message: ListItemData.message
                            replyAuthor: ListItemData.replyAuthor
                            replyMessage: ListItemData.replyMessage
                            image: ListItemData.image
                            imageLoading: ListItemData.imageLoading
                            imageLoadFailed: ListItemData.imageLoadFailed
                            imageWidth: ListItemData.imageWidth
                            imageHeight: ListItemData.imageHeight
                            attachmentUrl: ListItemData.attachmentUrl
                            attachmentName: ListItemData.attachmentName
                            attachmentIsImage: ListItemData.attachmentIsImage
                            isGroupStart: ListItemData.isGroupStart
                            isGroupEnd: ListItemData.isGroupEnd
                            showAvatar: ListItemData.showAvatar
                            showUsername: ListItemData.showUsername
                            showTimestamp: ListItemData.showTimestamp
                            pending: ListItemData.pending
                            failed: ListItemData.failed
                            edited: ListItemData.edited

                            onDeleteRequested: {
                                ListItem.view.deleteMessage(messageId);
                            }

                            onEditRequested: {
                                ListItem.view.startEditMessage(messageId, messageText);
                            }

                            onReplyRequested: {
                                ListItem.view.setReplyMessage(messageId, author, replyText);
                            }

                            onAttachmentOpenRequested: {
                                ListItem.view.openAttachment(url);
                            }

                            onAttachmentImageLoadRequested: {
                                ListItem.view.requestCachedImage(url);
                            }
                        }
                    }
                ]

                function itemType(data, indexPath) {
                    return "";
                }

                function startEditMessage(messageId, messageText) {
                    chatPage.startEdit(messageId, messageText);
                }

                function setReplyMessage(messageId, author, replyText) {
                    chatPage.setReply(messageId, author, replyText);
                }

                function requestOlderFromFirstItem(indexPath) {
                    chatPage.requestOlderFromFirstItem(indexPath);
                }

                function deleteMessage(messageId) {
                    chatController.deleteMessage(messageId);
                }

                function openAttachment(url) {
                    chatController.openAttachment(url);
                }

                function requestCachedImage(url) {
                    chatController.requestCachedImage(url);
                }

                attachedObjects: [
                    ListScrollStateHandler {
                        onAtBeginningChanged: {
                            if (atBeginning) {
                                chatPage.requestOlderFromScroll();
                            }
                        }
                    }
                ]
            }

            Container {
                visible: appStore.chatLoadingBefore
                preferredHeight: ui.du(7.0)
                horizontalAlignment: HorizontalAlignment.Fill
                verticalAlignment: VerticalAlignment.Top
                background: Color.create("#18191C")

                layout: DockLayout {}

                ActivityIndicator {
                    running: appStore.chatLoadingBefore
                    horizontalAlignment: HorizontalAlignment.Center
                    verticalAlignment: VerticalAlignment.Center
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

    function requestOlderFromFirstItem(indexPath) {
        if (!active || !indexPath || indexPath.length <= 0 || indexPath[0] !== 0) {
            return;
        }

        requestOlderFromScroll();
    }

    function requestOlderFromScroll() {
        if (!active) {
            return;
        }

        if (olderLoadRequested || chatController.isLoadingBefore() || !chatController.hasMoreBefore()) {
            return;
        }

        olderLoadRequested = true;
        chatController.requestOlderMessages();
    }

    function clearForChannelSwitch() {
        replyMessageId = "";
        replyAuthor = "";
        replyMessage = "";
        editingMessageId = "";
        olderLoadRequested = false;
    }

    function deactivatePage() {
        active = false;
    }

    function reactivatePage() {
        active = true;
    }

    onCreationCompleted: {
        active = true;
        channelName = chatController.currentChannelName;
        chatController.currentChannelChanged.connect(function () {
            chatPage.channelName = chatController.currentChannelName;
            chatPage.clearForChannelSwitch();
        });
        appStore.chatMessagesPrepended.connect(function (channelId, messages) {
            if (channelId == chatController.currentChannelId) {
                chatPage.olderLoadRequested = false;
            }
        });
        appStore.chatMessagesReset.connect(function (channelId, messages) {
            if (channelId == chatController.currentChannelId) {
                chatPage.olderLoadRequested = false;
            }
        });
    }
}
