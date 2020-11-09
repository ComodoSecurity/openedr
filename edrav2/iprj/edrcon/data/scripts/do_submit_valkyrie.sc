{
	"description": "Submit files to Valkyrie",
	"commands": [
		{
			"description": "Connect to the source queue",
			"command": {
				"processor": "objects.application",
				"command": "putCatalogData",
				"params": {
					"path": "objects.queueDoSubmitValkyrie",
					"data": {
						"$$proxy": "cachedCmd",
						"processor": "objects.queueManager",
						"command": "getQueue",
						"params": {
							"name": {
								"$$proxy": "cachedCmd",
								"processor": "objects.application",
								"command": "getCatalogData",
								"params": {
									"path": "app.config.script.check_for_valkyrie.submitQueue"
								}
							}
						}
					}
				}
			}
		},
		{
			"description": "Create a data generator for Valkyrie submitting queue",
			"command": {
				"processor": "objects.application",
				"command": "putCatalogData",
				"params": {
					"path": "objects.genDoSubmitValkyrie",
					"data": {
						"$$clsid": "0xB769174E", // CLSID_DataGenerator
						// "period": 10,
						"data": {
							"default": {
								"$$proxy": "cachedCmd",
								"processor": "objects.application",
								"command": "getCatalogData",
								"params": {
									"path": "objects.queueDoSubmitValkyrie"
								}
							}
						},
						"receiver": {
							"$$proxy": "cachedCmd",
							"processor": "objects.queueManager",
							"command": "getQueue",
							"params": {
								"name": "submit_to_valkyrie"
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
						"processor": "objects.genDoSubmitValkyrie",
						"command": "terminate",
						"params": {}
					}
				}
			}
		},
		{
			"description": "Start Valkyrie submission generator",
			"command": {
				"processor": "objects.genDoSubmitValkyrie",
				"command": "start",
				"params": {}
			}
		}
	]
}