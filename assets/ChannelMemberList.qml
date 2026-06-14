import bb.cascades 1.4

Page {
	id: memberPage

	property string channelName: "general"
	property alias title: titleBar.title

	signal backRequested()

	actionBarVisibility: ChromeVisibility.Hidden

	titleBar: TitleBar {
		id: titleBar
		title: qsTr("Members #") + memberPage.channelName
		visibility: ChromeVisibility.Visible

		dismissAction: ActionItem {
			imageSource: "asset:///images/icons/accent/caret-left.png"

			onTriggered: {
				memberPage.backRequested()
			}
		}
	}

	Container {
		horizontalAlignment: HorizontalAlignment.Fill
		verticalAlignment: VerticalAlignment.Fill

		layout: StackLayout {}

		ListView {
			id: memberList
				dataModel: channelMemberListController.memberDataModel
			horizontalAlignment: HorizontalAlignment.Fill
			verticalAlignment: VerticalAlignment.Fill

			listItemComponents: [
				ListItemComponent {
					type: "role"

					Container {
						horizontalAlignment: HorizontalAlignment.Fill
						leftPadding: ui.du(2.0)
						rightPadding: ui.du(2.0)
						topPadding: ui.du(2.2)
						bottomPadding: ui.du(0.6)

						Label {
							text: ListItemData.name + " — " + ListItemData.count
							opacity: 0.55
							textStyle.fontSize: FontSize.XSmall
							textStyle.fontWeight: FontWeight.Normal
							textStyle.color: Color.create("#B5BAC1")
						}
					}
				},

				ListItemComponent {
					type: "member"

					Container {
						horizontalAlignment: HorizontalAlignment.Fill
						leftPadding: ui.du(2.0)
						rightPadding: ui.du(2.0)
						topPadding: ui.du(0.8)
						bottomPadding: ui.du(0.8)
						property string lastAvatarRequestId: ""

						function requestAvatar() {
							if (lastAvatarRequestId == ListItemData.userId) {
								return;
							}
							if (ListItemData.avatar === "" && ListItemData.avatarHash !== "") {
								lastAvatarRequestId = ListItemData.userId;
								ListItem.view.loadVisibleMemberAvatar(ListItemData.userId, ListItemData.avatarHash);
							}
						}

						onCreationCompleted: {
							requestAvatar();
						}

						ListItem.onDataChanged: {
							if (lastAvatarRequestId != ListItemData.userId) {
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
							background: Color.create(ListItemData.avatarColor)

							layout: DockLayout {}

							ImageView {
								imageSource: ListItemData.avatar
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
							horizontalAlignment: HorizontalAlignment.Fill
							verticalAlignment: VerticalAlignment.Center
							leftMargin: ui.du(1.5)

							Label {
								text: ListItemData.name
								textStyle.fontSize: FontSize.Small
								textStyle.fontWeight: FontWeight.Normal
								textStyle.color: Color.create(ListItemData.nameColor)
							}

							Label {
								text: ListItemData.status
								topMargin: ui.du(-0.4)
								opacity: 0.6
								textStyle.fontSize: FontSize.XSmall
								textStyle.color: Color.create(ListItemData.statusColor)
							}
						}
					}
				},

				ListItemComponent {
					type: "loading"

					Container {
						horizontalAlignment: HorizontalAlignment.Fill
						leftPadding: ui.du(2.0)
						rightPadding: ui.du(2.0)
						topPadding: ui.du(1.5)
						bottomPadding: ui.du(1.5)

						ActivityIndicator {
							horizontalAlignment: HorizontalAlignment.Center
							preferredWidth: ui.du(4.0)
							preferredHeight: ui.du(4.0)
							running: true
						}
					}
				},

				ListItemComponent {
					type: "empty"

					Container {
						horizontalAlignment: HorizontalAlignment.Fill
						leftPadding: ui.du(2.0)
						rightPadding: ui.du(2.0)
						topPadding: ui.du(3.0)
						bottomPadding: ui.du(3.0)

						Label {
							text: ListItemData.text
							multiline: true
							horizontalAlignment: HorizontalAlignment.Center
							textStyle.fontSize: FontSize.Small
							textStyle.color: Color.create("#B5BAC1")
						}
					}
				}
			]

			function itemType(data, indexPath) {
				return data.type
			}

			function loadVisibleMemberAvatar(userId, avatarHash) {
				channelMemberListController.loadMemberAvatar(userId, avatarHash);
			}

			attachedObjects: [
				ListScrollStateHandler {
					onAtEndChanged: {
						if (atEnd) {
							channelMemberListController.loadMore();
						}
					}
				}
			]
		}
	}
}
