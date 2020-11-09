{
	"description": "Send events to the GCP",
	"reporter": "objects.genToGcp", // The name of the IReporterProvider object
	"commands": [
		{
			"description": "Create GCP output data generator",
			"command": {
				"processor": "objects.application",
				"command": "putCatalogData",
				"params": {
					"path": "objects.genToGcp",
					"data": {
						"$$clsid": "0xB769174E", // CLSID_DataGenerator
						//"period": 1000,
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
							"$$proxy": "cachedObj",
							"clsid": "0x51C1CAB6", // CLSID_GcpPubSubClient
							"saCredentials": {
								"$$proxy": "cachedCmd",
								"processor": "objects.application",
								"command": "getCatalogData",
								"params": { "path": "app.config.cloud.gcp.saCredentials" }
							},
							"pubSubTopic": {
								"$$proxy": "cachedCmd",
								"processor": "objects.application",
								"command": "getCatalogData",
								"params": { "path": "app.config.cloud.gcp.pubSubTopic" }
							},
							"caCertificates": {
								"$$proxy": "cmd",
								"processor": "objects.application",
								"command": "getConfigValue",
								"params": {
									"default": null,
									"paths": [ "app.config.caCertificates" ]
								}
							}
						}
					}
				}
			}
		},
		{
			"description": "Register stop handler for GCP output generator",
			"command": {
				"processor": "objects.application",
				"command": "subscribeToMessage",
				"params": {
					"id": "appScriptStop",
					"command": {
						"processor": "objects.genToGcp",
						"command": "terminate",
						"params": {}
					}
				}
			}
		},
		{
			"description": "Start output data generator",
			"command": {
				"processor": "objects.genToGcp",
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