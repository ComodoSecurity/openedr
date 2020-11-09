{
	"description": "Set specified directory as a data destination",
	"reporter": "objects.genToDir", // The name of the IReporterProvider object
	"commands": [
		{
			"description": "Create output data generator",
			"command": {
				"processor": "objects.application",
				"command": "putCatalogData",
				"params": {
					"path": "objects.genToDir",
					"data": {
						"$$clsid": "0xB769174E", // CLSID_DataGenerator
						//"period": 10,
						//"autoStop": true,
						"data": {
							"default": {
								"$$proxy": "cachedCmd",
								"processor": "objects.application",
								"command": "getCatalogData",
								"params": {
									"path": "objects.queueCurrOutput"
								}
							}
						},
						"receiver": {
							"$$clsid": "0xDA024E57", // CLSID_FsDataStorage
							"deleteOnGet": false,
							"mode": "create|truncate",
							"prettyOutput": true,
							"path": {
								"$$proxy": "cachedCmd",
								"processor": "objects.application",
								"command": "getCatalogData",
								"params": {
									"path": "app.params.script.out",
									"default": "to_dir_data"
								}
							}
						}
					}
				}
			}
		},
		{
			"description": "Register stop handler for output generator",
			"command": {
				"processor": "objects.application",
				"command": "subscribeToMessage",
				"params": {
					"id": "appScriptStop",
					"command": {
						"processor": "objects.genToDir",
						"command": "terminate",
						"params": {}
					}
				}
			}
		},
		{
			"description": "Start output data generator",
			"command": {
				"processor": "objects.genToDir",
				"command": "start",
				"params": {
					"count": {
						"$$proxy": "cachedCmd",
						"processor": "objects.application",
						"command": "getCatalogData",
						"params": {
							"path": "app.params.script.ocount",
							"default": -1
						}
					}
				}
			}
		}
	]
}