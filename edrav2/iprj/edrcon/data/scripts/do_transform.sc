{
	"description": "Transform event to cloud-frindly form v1.1",
	// "reporter": "objects.genCopy", // The name of the IReporterProvider object
	"commands": [
		{
			"description": "Get inpot queue for transforming",
			"command": {
				"processor": "objects.application",
				"command": "putCatalogData",
				"params": {
					"path": "objects.queueDoTransform",
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
			"description": "Create inout data generator for transforming queue",
			"command": {
				"processor": "objects.application",
				"command": "putCatalogData",
				"params": {
					"path": "objects.genDoTransform",
					"data": {
						"$$clsid": "0xB769174E", // CLSID_DataGenerator
						// "period": 10,
						"data": {
							"default": {
								"$$proxy": "cachedCmd",
								"processor": "objects.application",
								"command": "getCatalogData",
								"params": {
									"path": "objects.queueDoTransform"
								}
							}
						},
						"receiver": {
							"$$proxy": "cachedCmd",
							"processor": "objects.queueManager",
							"command": "getQueue",
							"params": {
								"name": "transform_for_cloud"
							}
						}
					}
				}
			}
		},
		{
			"description": "Initialize an output queue for transformed events",
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
							"name": "transform_for_cloud_out",
							"maxSize": 10000
						}
					}
				}
			}
		},
		{
			"description": "Set output queue for <transform_for_cloud> scenario",
			"command": {
				"processor": "objects.application",
				"command": "putCatalogData",
				"params": {
					"path": "app.config.script.transform_for_cloud.outputQueue",
					"data": "transform_for_cloud_out"
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
						"processor": "objects.genDoTransform",
						"command": "terminate",
						"params": {}
					}
				}
			}
		},
		{
			"description": "Start transformation generator",
			"command": {
				"processor": "objects.genDoTransform",
				"command": "start",
				"params": {}
			}
		}
	]
}