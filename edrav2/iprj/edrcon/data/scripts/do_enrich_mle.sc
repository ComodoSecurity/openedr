{
	"description": "Enrich MLE",
	// "reporter": "objects.genCopy", // The name of the IReporterProvider object
	"commands": [
		{
			"description": "Connect to the source queue",
			"command": {
				"processor": "objects.application",
				"command": "putCatalogData",
				"params": {
					"path": "objects.queueDoEnrichMLE",
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
			"description": "Create a data generator for MLE-enriching queue",
			"command": {
				"processor": "objects.application",
				"command": "putCatalogData",
				"params": {
					"path": "objects.genDoEnrichMLE",
					"data": {
						"$$clsid": "0xB769174E", // CLSID_DataGenerator
						// "period": 10,
						"data": {
							"default": {
								"$$proxy": "cachedCmd",
								"processor": "objects.application",
								"command": "getCatalogData",
								"params": {
									"path": "objects.queueDoEnrichMLE"
								}
							}
						},
						"receiver": {
							"$$proxy": "cachedCmd",
							"processor": "objects.queueManager",
							"command": "getQueue",
							"params": {
								"name": "enrich_mle"
							}
						}
					}
				}
			}
		},
		{
			"description": "Initialize an output queue for enriched MLE",
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
							"name": "enrich_mle_out",
							"maxSize": 10000
						}
					}
				}
			}
		},
		{
			"description": "Set output queue for <enrich_mle> scenario",
			"command": {
				"processor": "objects.application",
				"command": "putCatalogData",
				"params": {
					"path": "app.config.script.enrich_mle.outputQueue",
					"data": "enrich_mle_out"
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
						"processor": "objects.genDoEnrichMLE",
						"command": "terminate",
						"params": {}
					}
				}
			}
		},
		{
			"description": "Start MLE enriching generator",
			"command": {
				"processor": "objects.genDoEnrichMLE",
				"command": "start",
				"params": {}
			}
		}
	]
}