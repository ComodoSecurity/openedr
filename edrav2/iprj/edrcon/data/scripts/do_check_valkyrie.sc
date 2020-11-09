{
	"description": "Check events and return files to be submitted to Valkyrie",
	"commands": [
		{
			"description": "Connect to the source queue",
			"command": {
				"processor": "objects.application",
				"command": "putCatalogData",
				"params": {
					"path": "objects.queueDoCheckValkyrie",
					"data": {
						"$$proxy": "cachedCmd",
						"processor": "objects.application",
						"command": "getCatalogData",
						"params": {
							"path": "objects.queueCurrOutput"
						}
					}
				}
			}
		},
		{
			"description": "Create a data generator for Valkyrie checking queue",
			"command": {
				"processor": "objects.application",
				"command": "putCatalogData",
				"params": {
					"path": "objects.genDoCheckValkyrie",
					"data": {
						"$$clsid": "0xB769174E", // CLSID_DataGenerator
						// "period": 10,
						"data": {
							"default": {
								"$$proxy": "cachedCmd",
								"processor": "objects.application",
								"command": "getCatalogData",
								"params": {
									"path": "objects.queueDoCheckValkyrie"
								}
							}
						},
						"receiver": {
							"$$proxy": "cachedCmd",
							"processor": "objects.queueManager",
							"command": "getQueue",
							"params": {
								"name": "check_for_valkyrie"
							}
						}
					}
				}
			}
		},
		{
			"description": "Initialize an output queue",
			"command": {
				"processor": "objects.application",
				"command": "putCatalogData",
				"params": {
					"path": "objects.queueCurrOutput",
					"data": {
						"$$proxy": "cachedCmd",
						"processor": "objects.queueManager",
						"command": "addQueue",
						"params": {
							"name": "submit_to_valkyrie_in",
							"maxSize": 10000
						}
					}
				}
			}
		},
		{
			"description": "Set a submit queue for <check_for_valkyrie> scenario",
			"command": {
				"processor": "objects.application",
				"command": "putCatalogData",
				"params": {
					"path": "app.config.script.check_for_valkyrie.submitQueue",
					"data": "submit_to_valkyrie_in"
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
						"processor": "objects.genDoCheckValkyrie",
						"command": "terminate",
						"params": {}
					}
				}
			}
		},
		{
			"description": "Start the Valkyrie check generator",
			"command": {
				"processor": "objects.genDoCheckValkyrie",
				"command": "start",
				"params": {}
			}
		}
	]
}