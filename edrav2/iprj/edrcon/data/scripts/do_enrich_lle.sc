{
	"description": "Enrich LLE",
	// "reporter": "objects.genCopy", // The name of the IReporterProvider object
	"commands": [
		{
			"description": "Connect to the source queue",
			"command": {
				"processor": "objects.application",
				"command": "putCatalogData",
				"params": {
					"path": "objects.queueDoEnrichLLE",
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
			"description": "Create a data generator for LLE-enriching queue",
			"command": {
				"processor": "objects.application",
				"command": "putCatalogData",
				"params": {
					"path": "objects.genDoEnrichLLE",
					"data": {
						"$$clsid": "0xB769174E", // CLSID_DataGenerator
						// "period": 10,
						"data": {
							"default": {
								"$$proxy": "cachedCmd",
								"processor": "objects.application",
								"command": "getCatalogData",
								"params": {
									"path": "objects.queueDoEnrichLLE"
								}
							}
						},
						"receiver": {
							"$$proxy": "cachedCmd",
							"processor": "objects.queueManager",
							"command": "getQueue",
							"params": {
								"name": "enrich_lle"
							}
						}
					}
				}
			}
		},
		{
			"description": "Initialize an output queue for enriched LLE",
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
							"name": "enrich_lle_out",
							"maxSize": 10000
						}
					}
				}
			}
		},
		{
			"description": "Set output queue for <enrich_lle> scenario",
			"command": {
				"processor": "objects.application",
				"command": "putCatalogData",
				"params": {
					"path": "app.config.script.enrich_lle.outputQueue",
					"data": "enrich_lle_out"
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
						"processor": "objects.genDoEnrichLLE",
						"command": "terminate",
						"params": {}
					}
				}
			}
		},
		{
			"description": "Start LLE enriching generator",
			"command": {
				"processor": "objects.genDoEnrichLLE",
				"command": "start",
				"params": {}
			}
		}
	]
}