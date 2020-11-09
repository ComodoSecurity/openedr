{
	"description": "Send events to the AWS cloud",
	"reporter": "objects.genToAws", // The name of the IReporterProvider object
	"commands": [
		{
			"description": "Create AWS output data generator",
			"command": {
				"processor": "objects.application",
				"command": "putCatalogData",
				"params": {
					"path": "objects.genToAws",
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
							"$$clsid": "0x95BA0E24", // CLSID_AwsFirehoseClient
							"accessKey": {
								"$$proxy": "cachedCmd",
								"processor": "objects.application",
								"command": "getCatalogData",
								"params": { "path": "app.config.cloud.aws.accessKey" }
							},
							"secretKey": {
								"$$proxy": "cachedCmd",
								"processor": "objects.application",
								"command": "getCatalogData",
								"params": { "path": "app.config.cloud.aws.secretKey" }
							},
							"deliveryStream": {
								"$$proxy": "cachedCmd",
								"processor": "objects.application",
								"command": "getCatalogData",
								"params": { "path": "app.config.cloud.aws.deliveryStream" }
							}
						}
					}
				}
			}
		},
		{
			"description": "Register stop handler for AWS output generator",
			"command": {
				"processor": "objects.application",
				"command": "subscribeToMessage",
				"params": {
					"id": "appScriptStop",
					"command": {
						"processor": "objects.genToAws",
						"command": "terminate",
						"params": {}
					}
				}
			}
		},
		{
			"description": "Start output data generator",
			"command": {
				"processor": "objects.genToAws",
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