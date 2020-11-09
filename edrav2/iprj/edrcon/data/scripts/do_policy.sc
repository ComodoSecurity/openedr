{
	"description": "Apply policy filtering for event",
	// "reporter": "objects.genCopy", // The name of the IReporterProvider object
	"commands": [
		{
			"description": "Get input queue for applying policy",
			"command": {
				"processor": "objects.application",
				"command": "putCatalogData",
				"params": {
					"path": "objects.queueDoPolicy",
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
			"description": "Create input data generator for policy applying queue",
			"command": {
				"processor": "objects.application",
				"command": "putCatalogData",
				"params": {
					"path": "objects.genDoPolicy",
					"data": {
						"$$clsid": "0xB769174E", // CLSID_DataGenerator
						// "period": 10,
						"data": {
							"default": {
								"$$proxy": "cachedCmd",
								"processor": "objects.application",
								"command": "getCatalogData",
								"params": {
									"path": "objects.queueDoPolicy"
								}
							}
						},
						"receiver": {
							"$$proxy": "cachedCmd",
							"processor": "objects.queueManager",
							"command": "getQueue",
							"params": {
								"name": "apply_policy"
							}
						}
					}
				}
			}
		},
		{
			"description": "Initialize an output queue for approved events",
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
							"name": "apply_policy_out",
							"maxSize": 10000
						}
					}
				}
			}
		},
		{
			"description": "Set output queue for <apply_policy> scenario",
			"command": {
				"processor": "objects.application",
				"command": "putCatalogData",
				"params": {
					"path": "app.config.script.apply_policy.outputQueue",
					"data": "apply_policy_out"
				}
			}
		},
		{
			"description": "Register stop handler for input data generator",
			"command": {
				"processor": "objects.application",
				"command": "subscribeToMessage",
				"params": {
					"id": "appScriptStop",
					"command": {
						"processor": "objects.genDoPolicy",
						"command": "terminate",
						"params": {}
					}
				}
			}
		},
		{
			"description": "Start transformation generator",
			"command": {
				"processor": "objects.genDoPolicy",
				"command": "start",
				"params": {}
			}
		}
	]
}