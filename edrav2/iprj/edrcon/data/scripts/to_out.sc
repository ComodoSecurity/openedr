{
	"description": "Set output scenario as an output",
	"reporter": "objects.genToOut", // The name of the IReporterProvider object
	"commands": [
		{
			"description": "Create output data generator",
			"command": {
				"processor": "objects.application",
				"command": "putCatalogData",
				"params": {
					"path": "objects.genToOut",
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
							"$$proxy": "cachedCmd",
							"processor": "objects.queueManager",
							"command": "getQueue",
							"params": {
								"name": "output"
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
						"processor": "objects.genToOut",
						"command": "terminate",
						"params": {}
					}
				}
			}
		},
		{
			"description": "Start output data generator",
			"command": {
				"processor": "objects.genToOut",
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