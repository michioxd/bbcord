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
			var item = dmModel.data(indexPath)

			if (item.type == "dm") {
				dmList.dmSelected(item.name)
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
				type: "category"

				Container {
					preferredHeight: 2.0
					horizontalAlignment: HorizontalAlignment.Fill
					leftPadding: ui.du(2)
					rightPadding: ui.du(2)
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
				type: "dm"

				Container {
					preferredHeight: ui.du(8.0)
					horizontalAlignment: HorizontalAlignment.Fill
					leftPadding: ui.du(2)
					rightPadding: ui.du(2)

					function requestAvatar() {
						if (ListItemData.avatar === "" && ListItemData.avatarHash !== "") {
							ListItem.view.loadVisibleDmAvatar(ListItemData.id)
						}
					}

					onCreationCompleted: {
						requestAvatar()
					}

					ListItem.onDataChanged: {
						requestAvatar()
					}

					layout: StackLayout {
						orientation: LayoutOrientation.LeftToRight
					}

					Container {
						preferredWidth: ui.du(6.0)
						preferredHeight: ui.du(6.0)
						minWidth: ui.du(6.0)
						minHeight: ui.du(6.0)
						maxWidth: ui.du(6.0)
						maxHeight: ui.du(6.0)
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
			return data.type
		}

		function loadVisibleDmAvatar(channelId) {
			dmListController.loadDmAvatar(channelId)
		}

		attachedObjects: [
			ListScrollStateHandler {
				onAtEndChanged: {
					if (atEnd) {
						dmListController.loadMoreDmChannels()
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
		dmModel.clear()
		dmModel.append({"type": "category", "name": "Direct Messages"})
		for (var i = 0; i < appStore.dmChannels.length; ++i) {
			dmModel.append(appStore.dmChannels[i])
		}
		updateDmLoading()
	}

	function appendDms(channels) {
		removeDmLoading()
		for (var i = 0; i < channels.length; ++i) {
			dmModel.append(channels[i])
		}
		updateDmLoading()
	}

	function refreshDmsIfNeeded() {
		var end = dmModel.size() - loadingRowOffset()
		if (end - 1 != appStore.dmChannels.length) {
			refreshDms()
			return
		}

		for (var i = 0; i < appStore.dmChannels.length; ++i) {
			var item = dmModel.data([i + 1])
			if (!item || item.id != appStore.dmChannels[i].id) {
				refreshDms()
				return
			}
		}
	}

	function loadingRowOffset() {
		if (dmModel.size() == 0) {
			return 0
		}

		var last = dmModel.data([dmModel.size() - 1])
		return last.type == "loading" ? 1 : 0
	}

	function removeDmLoading() {
		if (dmModel.size() == 0) {
			return
		}

		var lastIndex = dmModel.size() - 1
		var last = dmModel.data([lastIndex])
		if (last.type == "loading") {
			dmModel.removeAt([lastIndex])
		}
	}

	function updateDmLoading() {
		removeDmLoading()
		if (appStore.dataLoading) {
			dmModel.append({ "type": "loading" })
		}
	}

	onCreationCompleted: {
		refreshDms()
		appStore.dmChannelsAppended.connect(appendDms)
		appStore.dmAvatarChanged.connect(updateDmAvatar)
		appStore.dmChannelsChanged.connect(refreshDmsIfNeeded)
		appStore.dataLoadingChanged.connect(updateDmLoading)
	}

	function updateDmAvatar(channelId, avatarSource) {
		if (dmModel.size() == 0) return
		var end = dmModel.size() - loadingRowOffset()
		for (var i = 1; i < end; ++i) {
			var item = dmModel.data([i])
			if (item && item.id == channelId) {
				item.avatar = avatarSource
				dmModel.replace([i], item)
				break
			}
		}
	}
}
