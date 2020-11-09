{
	"description": "Set specified directory as a data destination",
	"reporter": "objects.genToFile", // The name of the IReporterProvider object
	"commands": [
		{
			"description": "Create file output data generator",
			"command": {
				"processor": "objects.application",
				"command": "putCatalogData",
				"params": {
					"path": "objects.genToFile",
					"data": {
						"$$clsid": "0xB769174E", // CLSID_DataGenerator
						// "period": 10,
						// "autoStop": true,
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
							"$$clsid": "0xCEFC4454", // CLSID_File
							"path": {
								"$$proxy": "cachedCmd",
								"processor": "objects.application",
								"command": "getCatalogData",
								"params": {
									"path": "app.params.script.out",
									"default": "to_file_data"
								}
							},
							"putDelimiter": "\n\n",
							"mode": "wtRW"
						}
					}
				}
			}
		},
		{
			"description": "Register stop handler for file output data generator",
			"command": {
				"processor": "objects.application",
				"command": "subscribeToMessage",
				"params": {
					"id": "appScriptStop",
					"command": {
						"processor": "objects.genToFile",
						"command": "terminate",
						"params": {}
					}
				}
			}
		},
		{
			"description": "Start file output data generator",
			"command": {
				"processor": "objects.genToFile",
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