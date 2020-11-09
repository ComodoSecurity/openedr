{
	"description": "Set specified directory as a data source",
	"reporter": "objects.genFromDir", // The name of the IReporterProvider object
	"commands": [
		{
			"description": "Create input data generator",
			"command": {
				"processor": "objects.application",
				"command": "putCatalogData",
				"params": {
					"path": "objects.genFromDir",
					"data": {
						"$$clsid": "0xB769174E", // CLSID_DataGenerator
						//"period": 1000,
						"autoStop": true,
						"data": {
							"default": {
								"$$clsid": "0xDA024E57", // CLSID_FsDataStorage
								"deleteOnGet": false,
								"path": {
									"$$proxy": "cachedCmd",
									"processor": "objects.application",
									"command": "getCatalogData",
									"params": {
										"path": "app.params.script.in",
										"default": "from_dir_data"
									}
								}
							}
						},
						"receiver": {
							"$$proxy": "cachedCmd",
							"processor": "objects.queueManager",
							"command": "getQueue",
							"params": {
								"name": "inbox"
							}
						}
					}
				}
			}
		},
		{
			"description": "Register stop handler",
			"command": {
				"processor": "objects.application",
				"command": "subscribeToMessage",
				"params": {
					"id": "appScriptStop",
					"command": {
						"processor": "objects.genFromDir",
						"command": "stop",
						"params": {}
					}
				}
			}
		},
		{
			"description": "Initialize output queue for current generator",
			"command": {
				"processor": "objects.application",
				"command": "putCatalogData",
				"params": {
					"path": "objects.queueCurrOutput",
					"data": {
						"$$proxy": "cachedCmd",
						"processor": "objects.queueManager",
						"command": "getQueue",
						"params": {
							"name": "inbox"
						}
					}
				}
			}
		},
		{
			"description": "Start input data generator",
			"command": {
				"processor": "objects.genFromDir",
				"command": "start",
				"params": {
					"count": {
						"$$proxy": "cachedCmd",
						"processor": "objects.application",
						"command": "getCatalogData",
						"params": {
							"path": "app.params.script.icount",
							"default": -1
						}
					}
				}
			}
		}
	]
}
