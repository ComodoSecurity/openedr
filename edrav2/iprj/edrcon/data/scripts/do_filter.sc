{
	"description": "Filter low-level events by specified id's",
	// "reporter": "objects.genFilter", // The name of the IReporterProvider object
	"commands": [
		{
			"description": "Get input queue for filtering",
			"command": {
				"processor": "objects.application",
				"command": "putCatalogData",
				"params": {
					"path": "objects.queueDoFilter",
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
		// Initialize filter 'type' settings
		{
			"description": "Initialize filter settings (type)",
			"command": {
				"processor": "objects.application",
				"command": "putCatalogData",
				"params": {
					"path": "app.config.script.filter_lle.type",
					"data": {
						"$$proxy": "cachedCmd",
						"processor": "objects.application",
						"command": "getCatalogData",
						"params": {
							"path": "app.params.script.p1",
							"default": 0
						}
					}
				}
			}
		},
		// Initialize filter 'pid' settings
		{
			"description": "Initialize filter settings (pid)",
			"command": {
				"processor": "objects.application",
				"command": "putCatalogData",
				"params": {
					"path": "app.config.script.filter_lle.pid",
					"data": {
						"$$proxy": "cachedCmd",
						"processor": "objects.application",
						"command": "getCatalogData",
						"params": {
							"path": "app.params.script.p2",
							"default": 2
						}
					}
				}
			}
		},
		{
			"description": "Create input data generator for filtering queue",
			"command": {
				"processor": "objects.application",
				"command": "putCatalogData",
				"params": {
					"path": "objects.genDoFilter",
					"data": {
						"$$clsid": "0xB769174E", // CLSID_DataGenerator
						// "period": 10,
						"data": {
							"default": {
								"$$proxy": "cachedCmd",
								"processor": "objects.application",
								"command": "getCatalogData",
								"params": {
									"path": "objects.queueDoFilter"
								}
							}
						},
						"receiver": {
							"$$proxy": "cachedCmd",
							"processor": "objects.queueManager",
							"command": "getQueue",
							"params": {
								"name": "filter_lle"
							}
						}
					}
				}
			}
		},
		{
			"description": "Initialize output queue for filtered events",
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
							"name": "filter_lle_out",
							"maxSize": 10000
						}
					}
				}
			}
		},
		{
			"description": "Set output queue for <filter_lle> scenario",
			"command": {
				"processor": "objects.application",
				"command": "putCatalogData",
				"params": {
					"path": "app.config.script.filter_lle.outputQueue",
					"data": "filter_lle_out"
				}
			}
		},
		{
			"description": "Register stop handler for data filtering input generator",
			"command": {
				"processor": "objects.application",
				"command": "subscribeToMessage",
				"params": {
					"id": "appScriptStop",
					"command": {
						"processor": "objects.genDoFilter",
						"command": "terminate",
						"params": {}
					}
				}
			}
		},
		{
			"description": "Start filtering input generator",
			"command": {
				"processor": "objects.genDoFilter",
				"command": "start",
				"params": {}
			}
		}
	]
}