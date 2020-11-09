//
// Parameters:
//  * p1 - a batch size (default: 100)
//
{
	"description": "Group events into batches",
	"commands": [
		{
			"description": "Get an input queue for processing",
			"command": {
				"processor": "objects.application",
				"command": "putCatalogData",
				"params": {
					"path": "objects.queueDoBatch",
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
			"description": "Create a batch generator",
			"command": {
				"processor": "objects.application",
				"command": "putCatalogData",
				"params": {
					"path": "objects.genDoBatch",
					"data": {
						"$$clsid": "0xB769174E", // CLSID_DataGenerator
						"data": {
							"default": {
								"$$proxy": "cachedCmd",
								"processor": "objects.application",
								"command": "getCatalogData",
								"params": {
									"path": "objects.queueDoBatch"
								}
							}
						},
						"receiver": {
							"$$proxy": "cachedCmd",
							"processor": "objects.queueManager",
							"command": "addQueue",
							"params": {
								"name": "batch_out",
								"maxSize": 10000,
								"batchSize": {
									"$$proxy": "cachedCmd",
									"processor": "objects.application",
									"command": "getCatalogData",
									"params": {
										"path": "app.params.script.p1",
										"default": 100
									}
								},
								// Disable batching via timeout
								"batchTimeout": 0
							}
						}
					}
				}
			}
		},
		{
			"description": "Initialize the output queue",
			"command": {
				"processor": "objects.application",
				"command": "putCatalogData",
				"params": {
					"path": "objects.queueCurrOutput",
					"data": {
						"$$proxy": "cachedCmd",
						"processor": "objects.queueManager",
						"command": "getQueue",
						"params": {
							"name": "batch_out"
						}
					}
				}
			}
		},
		{
			"description": "Register the stop handler",
			"command": {
				"processor": "objects.application",
				"command": "subscribeToMessage",
				"params": {
					"id": "appScriptStop",
					"command": {
						"processor": "objects.genDoBatch",
						"command": "terminate",
						"params": {}
					}
				}
			}
		},
		{
			"description": "Start the generator",
			"command": {
				"processor": "objects.genDoBatch",
				"command": "start",
				"params": {}
			}
		}
	]
}
