{
	"description": "Set null device as a data destination",
	"reporter": "objects.genToNull", // The name of the IReporterProvider object
	"commands": [
		{
			"description": "Create file output data generator",
			"command": {
				"processor": "objects.application",
				"command": "putCatalogData",
				"params": {
					"path": "objects.genToNull",
					"data": {
						"$$clsid": "0xB769174E", // CLSID_DataGenerator
						//"period": 10,
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
							"$$clsid": "0x2CFF4596" // CLSID_NullStream
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
						"processor": "objects.genToNull",
						"command": "terminate",
						"params": {}
					}
				}
			}
		},
		{
			"description": "Start file output data generator",
			"command": {
				"processor": "objects.genToNull",
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