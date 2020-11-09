{
	"description": "Record the set of events from System Monitor",
	// "reporter": "objects.genCopy", // The name of the IReporterProvider object
	"commands": [
		{
			"description": "Save current queue",
			"command": {
				"processor": "objects.application",
				"command": "putCatalogData",
				"params": {
				"path": "objects.queueDoCopy",
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
			"description": "Create copy data generator",
			"command": {
				"processor": "objects.application",
				"command": "putCatalogData",
				"params": {
				"path": "objects.genDoCopy",
					"data": {
						"$$clsid": "0xB769174E", // CLSID_DataGenerator
						// "period": 10,
						//"autoStop": true,
						"data": {
							"default": {
								"$$proxy": "cachedCmd",
								"processor": "objects.application",
								"command": "getCatalogData",
								"params": {
									"path": "objects.queueDoCopy"
								}
							}
						},
            "receiver": {
							"$$proxy": "cachedCmd",
							"processor": "objects.queueManager",
							"command": "addQueue",
							"params": {
								"name": {
									"$$proxy": "cachedCmd",
									"processor": "objects.application",
									"command": "getCatalogData",
									"params": {
										"path": "app.params.script.p1",
										"default": "outbox"
									}
								},
								"maxSize": 10000
							}
						}
					}
				}
			}
		},
		{
			"description": "Initialize output queue for copy generator",
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
							"name": {
								"$$proxy": "cachedCmd",
								"processor": "objects.application",
								"command": "getCatalogData",
								"params": {
									"path": "app.params.script.p1",
									"default": "outbox"
								}
							}
						}
					}
				}
			}
		},
		{
			"description": "Register copy generator stop handler",
			"command": {
				"processor": "objects.application",
				"command": "subscribeToMessage",
				"params": {
					"id": "appScriptStop",
					"command": {
						"processor": "objects.genDoCopy",
						"command": "terminate",
						"params": {}
					}
				}
			}
		},
		{
			"description": "Start data copy generator",
			"command": {
			"processor": "objects.genDoCopy",
			"command": "start",
				"params": {}
			}
		}
	]
}