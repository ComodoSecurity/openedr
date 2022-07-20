========================================================================
    CONSOLE APPLICATION : TrafficShaperWFP Project Overview
========================================================================

	Limits the bandwidth for the specified process

	Usage: TrafficShaperWFP.exe <process name> <limit>
	<process name> - short process name, e.g. firefox.exe
	<limit> - network IO limit in bytes per second for all instances of the specified process

	Example: TrafficShaperWFP.exe firefox.exe 10000
	Limits TCP/UDP traffic to 10000 bytes per second for all instances of Firefox

	The sample works only with WFP driver.