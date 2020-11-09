function MsgBox(msg) {
	var r = Session.Installer.CreateRecord(0);
	r.StringData(0) = msg;
	Session.Message(0x01000000 | 0x00000010, r);
}

function MsgBoxOk(msg) {
	var r = Session.Installer.CreateRecord(0);
	r.StringData(0) = msg;
	Session.Message(0x03000000 | 0x00000040, r);
}

function Log(msg) {
	var r = Session.Installer.CreateRecord(1);
	r.StringData(0) = "Log: [1]";
	r.StringData(1) = msg;
	Session.Message(0x04000000, r);
}

function LogErr(err) {
	Log("Error: " + err.description + "(" + err.number + ")");
}

function CleanInstallDirectory() {
	var params = Session.Property("CustomActionData").split("*");
	var installDir = params[0];
	Log("Clean directory: " + installDir);
	var fso = new ActiveXObject("Scripting.FileSystemObject");
	try {
		fso.DeleteFolder(installDir + "*", true);
	}
	catch (err) {
		LogErr(err);
	}
	try {
		fso.DeleteFile(installDir + "*", true);
	}
	catch (err) {
		LogErr(err);
	}
}

function OfferRepairing() {
	Log("OfferRepairing");
	var r = Session.Installer.CreateRecord(0);
	r.StringData(0) = "Product is installed already. Would you like to repair it?";
	var result = Session.Message(0x03000000 | 0x00000004 | 0x00000020, r);
	if (result == 7) {
		// NO
		return 2;
	}
	else {
		Session.Property("REINSTALL") = "ALL";
		Session.Property("REINSTALLMODE") = "amusv";
		Session.Property("USER_REPAIR") = "1";
	}
	return 1;
}

function GetInstallCmdLine() {
	var cmdLine = [];
	//var r = Session.Installer.CreateRecord(0);
	//r.StringData(0) = "REINSTALLMODE = " + Session.Property("REINSTALLMODE") + "\nREINSTALL = " + Session.Property("REINSTALL");
	//Session.Message(0x01000000 | 0x00000010, r);

	var token = Session.Property("TOKEN");
	if (!token) token = _ExtractToken(Session.Property("OriginalDatabase"));

	var agentId = OldAgentValues.getAgentId();
	var oldMid = OldAgentValues.getMid();
	var oldInstallDate = OldAgentValues.getInstallDate();
	var oldUniqueGuid = OldAgentValues.getUniqueGuid();

	if (Session.Property("ROOT_URL")) {
		cmdLine.push("--rootUrl=\"" + Session.Property("ROOT_URL") + "\"");
	}

	if (oldMid != null) {
		if (oldInstallDate != null && oldUniqueGuid != null) cmdLine.push("--machineId=\"" + oldMid + "[" + oldInstallDate + "]" + oldUniqueGuid + "\"");
		else cmdLine.push("--machineId=\"" + oldMid + "\"");
	}
	else if (agentId != null) {
		cmdLine.push("--machineId=\"" + agentId + "\"");
	}

	if (token == null) {
		var customerId = OldAgentValues.getCustomerId();
		if (customerId != null) {
			cmdLine.push("--legacyClientId=\"" + customerId + "\"");
		}
	}

	if (token != null) {
		cmdLine.push("--token=\"" + token + "\"");
	}
	else if (Session.Property("DISABLE") != 1 && !Session.Property("Installed") && !Session.Property("REMOVE") && !Session.Property("WIX_UPGRADE_DETECTED") && !Session.Property("EDRUPGRADE")) {
		//When EDR became opensource, null token means local mode;
		Log("Set DISABLE=1 to skip enroll step.");
		Session.Property("DISABLE") = "1";
		SetLocalMode();
	}

	Session.Property("SvcInstallCommandLine") = cmdLine.join(" ");

	return 1;
}

var OldAgentValues = {
	"__readOldAgentValue": function (name) {
		var shell = new ActiveXObject("WScript.Shell");
		try {
			return shell.RegRead("HKEY_LOCAL_MACHINE\\SOFTWARE\\Wow6432Node\\OpenEdr\\EDREndpoint\\" + name);
		}
		catch (err) {
			LogErr(err);
		}
		try {
			return shell.RegRead("HKEY_LOCAL_MACHINE\\SOFTWARE\\OpenEdr\\EDREndpoint\\" + name);
		}
		catch (err) {
			LogErr(err);
		}
		return null;
	},

	"__WmiReadReg": function (root, key, valueName, valueType) {
		var value = null;
		try {
			value = WmiReadRegStr(root, key, valueName, valueType, 32);
			Log("get value -> " + valueName + " " + value);
			if (value == 0) value = null;
		}
		catch (err) {
			LogErr(err);
		}
		if (value && value != null) return value;

		try {
			value = WmiReadRegStr(root, key, valueName, valueType, 64);
			Log("get value -> " + valueName + " " + value);
			if (value == 0) value = null;
		}
		catch (err) {
			LogErr(err);
		}
		if (value && value != null) return value;
	},

	"getAgentId": function () {
		if (Session.Property("OLDAGENT1INSTALLED")) {
			return this.__readOldAgentValue("AgentID");
		}
		return null;
	},

	"getMid": function () {
		if (Session.Property("OLDAGENT1INSTALLED")) {
			return this.__readOldAgentValue("mid");
		}
		return null;
	},

	"getCustomerId": function () {
		if (Session.Property("OLDAGENT1INSTALLED")) {
			return this.__readOldAgentValue("customerID");
		}
		return null;
	},

	"getInstallDate": function () {
		if (Session.Property("OLDAGENT1INSTALLED")) {
			var value = this.__WmiReadReg(0x80000002, "Software\\Microsoft\\Windows NT\\CurrentVersion", "InstallDate", "REG_DWORD");
			Log("getInstallDate -> " + value);
			return value;
		}
	},

	"getUniqueGuid": function () {
		if (Session.Property("OLDAGENT1INSTALLED")) {
			var value = this.__WmiReadReg(0x80000002, "Software\\Microsoft\\Windows NT\\CurrentVersion", "uniqueEDRGuid", "REG_SZ");
			Log("getUniqueGuid -> " + value);
			return value;
		}
	}
};

function WmiReadRegStr(RootKey, KeyName, ValueName, ValueType, ArchType) {
	var methodName = "GetStringValue";
	if (ValueType == "REG_DWORD") methodName = "GetDWORDValue";
	var context = new ActiveXObject("WbemScripting.SWbemNamedValueSet");
	context.Add("__ProviderArchitecture", ArchType);
	context.Add("__RequiredArchitecture", true);

	var locator = new ActiveXObject("Wbemscripting.SWbemLocator");
	var wbem = locator.ConnectServer(null, "root\\default", null, null, null, null, null, context);
	var StdRegProv = wbem.Get("StdRegProv");
	var method = StdRegProv.Methods_.Item(methodName);
	var inParameters = method.InParameters.SpawnInstance_();
	inParameters.hDefKey = RootKey;
	inParameters.sSubKeyName = KeyName;
	inParameters.sValueName = ValueName;
	var outParameters = StdRegProv.ExecMethod_(methodName, inParameters, 0, context);

	if (ValueType == "REG_DWORD") return outParameters.UValue;
	return outParameters.SValue;
}

function SaveToken() {
	var params = Session.Property("CustomActionData").split("*");
	var token = params[0];
	if (token) {
		var cmdLine = "reg add HKLM\\Software\\OpenEdrAgent /v Token /d \"" + token + "\" /f";
		if (params[1]) {
			cmdLine += " /reg:64";
		}
		var res = SilentRun(cmdLine);
		Log("Save token to registry. Result = " + res.returnCode + ", output: " + res.output);
	}
	else {
		Log("Token was not saved because it is empty");
	}
}

function SetLocalMode() {
	var cmdLine = "reg add HKLM\\Software\\OpenEdrAgent /v LocalMode /t REG_DWORD /d 1 /f";
	var res = SilentRun(cmdLine);
	Log("Save LocalMode=1 to registry");
}


function _ExtractToken(path) {
	res = (/_([^_\.]*)\.msi$/ig).exec(path);
	if (res!= null && res.length > 1) return res[1];
	return null;
}

function RemoveExistingProducts() {
	var actionDataArr = Session.Property("CustomActionData").split("*"); // [WIX_UPGRADE_DETECTED]*[UILevel]*[OriginalDatabase]*[REINSTALL]*[DISABLE]*[UPDATE]*[TOKEN]*[ROOT_URL]
	var needReboot = false;
	if (!(actionDataArr[3])) {
		if (_CheckCmdNf()) needReboot = true;
		if (_CheckCwagtSrv()) needReboot = true;
		_CheckCmdCwagt(); // ignore old driver
	}
	if (_CheckEdrSvc()) needReboot = true;
	if (_CheckEdrDrv()) needReboot = true;

	Log("Delete next products: " + actionDataArr[0]);
	DeleteOldProducts(actionDataArr[0].split(";"));
	
	if (needReboot) {
		Log("Need reboot to continue installation");
		InterruptAndRestartAfterReboot(actionDataArr[1], actionDataArr[2], actionDataArr[4], actionDataArr[5], true, actionDataArr[6], actionDataArr[7]);
	}
	//MsgBox("RemoveExistingProducts -> " + needReboot);
	if (needReboot) return 3;
	return 1;
}

function DeleteOldProducts(products) {
	for (var i = 0; i < products.length; ++i) {
		if (products[i].length > 0) _CheckProduct(products[i]);  // just delete product and do not require reboot
	}
}

function InterruptAndRestartAfterReboot(uiLevel, path, disableProperty, updateProperty, upgrade, token, rootUrl) {
	var shell = new ActiveXObject("WScript.Shell");
	var cmdLine = "msiexec /i " + path;
	if (uiLevel == 2) cmdLine += " /q";
	
	if (upgrade) cmdLine += " EDRUPGRADE=1";
	if (disableProperty == 1) cmdLine += " DISABLE=1";
	if (updateProperty == 1) cmdLine += " UPDATE=1";
	if (token) cmdLine += " TOKEN=" + token;
	if (rootUrl) cmdLine += " ROOT_URL=" + rootUrl;
	shell.RegWrite("HKLM\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\RunOnce\\InstallEdrNeedReboot", cmdLine);
	
	shell.RegDelete("HKLM\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\RunOnce\\InstallEdr");
	return 3;
}

function _CheckCmdNf() {
	return _CheckService("CmdNf");
}

function _CheckCwagtSrv() {
	return _CheckService("cwagtsrv");
}

function _CheckCmdCwagt() {
	return _CheckService("CmdCwagt");
}

function _CheckEdrDrv() {
	return _CheckService("edrdrv");
}

function _CheckEdrSvc() {
	return _CheckService("edrsvc");
}

function _CheckProduct(product) {
	// clean uninstall section
	Log("Clean registry for product (uninstall) " + product);
	RegDelete("HKLM\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\" + product + "\\", true);  // delete x64 product
	RegDelete("HKLM\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\" + product + "\\", false);  // delete x86 product
	// convert product id to hash

	var parts = product.substring(1, product.length - 1).split("-");
	var hash = null;
	if (parts.length == 5) {
		try {
			parts[0] = parts[0].split("").reverse().join("");
			parts[1] = parts[1].split("").reverse().join("");
			parts[2] = parts[2].split("").reverse().join("");

			function process(s) {
				var new_s = ""
				for (var i = 0; i < s.length; i += 2) {
					new_s += s.charAt(i + 1) + s.charAt(i);
				}
				return new_s;
			}
			parts[3] = process(parts[3]);
			parts[4] = process(parts[4]);
			hash = parts.join("");
		}
		catch (err) {
			LogErr(err);
		}
	}
	if (hash != null) {
		Log("Clean registry for product (Features, Products) " + hash);
		// clean feature section
		RegDelete("HKLM\\SOFTWARE\\Classes\\Installer\\Features\\" + hash + "\\", true);  // delete x64 product
		RegDelete("HKLM\\SOFTWARE\\Classes\\Installer\\Features\\" + hash + "\\", false);  // delete x86 product
		// clean product section
		RegDelete("HKLM\\SOFTWARE\\Classes\\Installer\\Products\\" + hash + "\\", true);  // delete x64 product
		RegDelete("HKLM\\SOFTWARE\\Classes\\Installer\\Products\\" + hash + "\\", false);  // delete x86 product
	}
}

function _CheckService(service) {
	Log("Start checking service: " + service);
	var runResult = SilentRun("sc query " + service);
	Log("Service control '" + service + "': " + runResult.returnCode);
	Log(runResult.output);
	if (runResult.returnCode == 0) {
		Log("Service " + service + " was found");
		var stopRes = SilentRun("net stop " + service);
		Log("Stop service " + service + ": " + stopRes.returnCode + ", output: " + stopRes.output);
		var delRes = SilentRun("sc delete " + service);
		Log("Delete service " + service + ": " + delRes.returnCode + ", output: " + delRes.output);
		// RegDelete("HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\services\\" + service + "\\", true);
		RegDelete("HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\services\\" + service + "\\", false);

		if (delRes.returnCode == 0) {
			Log("Service " + service + " deleted succesfully");
			return false;
		}
		if (delRes.returnCode == 1072) {
			Log("Service " + service + " is marked to delete");
			return false;
		}
		return true;
	}
	Log("Service " + service + " was not found");
	return false;
}

function SilentRun(cmdLine) {
	var shell = new ActiveXObject("WScript.Shell");
	var fso = new ActiveXObject("Scripting.FileSystemObject");
	var fullFileName = null;

	try {
		var tmpFolder = fso.GetSpecialFolder(2);
		var tmpName = fso.GetTempName();
		var tmpFile = tmpFolder.CreateTextFile(tmpName);
		var fullFileName = tmpFolder.Path + "\\" + tmpName;
		tmpFile.Close();
	}
	catch (err) { }
	//"C:\Windows\System32\cmd.exe" /C "C:\Program Files\OpenEdr\EdrAgentV2\edrsvc.exe" install  > C:\Users\Tester\AppData\Local\Temp\rad0AFF3.tmp 2>&1
	//"C:\Windows\System32\cmd.exe" /C "C:\Program Files\OpenEdr\EdrAgentV2\edrsvc.exe" install  > C:\Users\Tester\AppData\Local\Temp\rad177B7.tmp 2>&1
	if (fullFileName != null) cmdLine = "cmd /C \"" + cmdLine + "\" > " + fullFileName + " 2>&1";
	
	Log("Run command line: " + cmdLine);
	var retCode = shell.Run(cmdLine, 0, true);
	var output = "";
	try {
		var stream = fso.OpenTextFile(fullFileName, 1, false, 0);
		output = stream.ReadAll();
		stream.Close();
	}
	catch (err) { }
	try {
		fso.DeleteFile(fullFileName);
	}
	catch (err) { }

	return {
		"returnCode": retCode,
		"output": output 
	};
}

function RegDelete(path, x64) {
	var cmdLine = "reg delete " + path + " /f";
	if (x64) cmdLine += " /reg:64";
	else cmdLine += " /reg:32";
	var res = SilentRun(cmdLine);
	Log("Delete registry key: " + path + ", result: " + res.returnCode + ", output: " + res.output);
}

function RegDeleteValue(path, value, x64) {
	var cmdLine = "reg delete " + path + " /v " + value + " /f";
	if (x64) cmdLine += " /reg:64";
	else cmdLine += " /reg:32";
	var res = SilentRun(cmdLine);
	Log("Delete registry key: " + path + ", result: " + res.returnCode + ", output: " + res.output);
}

function CheckUninstall() {
	_CheckEdrDrv();
	_CheckEdrSvc();
	return 1;
}

function OnStartAdminInstallation() {
	var shell = new ActiveXObject("WScript.Shell");

	try {
		var cmdLine = "msiexec /i " + Session.Property("OriginalDatabase");
		if (Session.Property("REINSTALL")) cmdLine += " REINSTALL=" + Session.Property("REINSTALL"); // + args[1];
		if (Session.Property("REINSTALLMODE")) cmdLine += " REINSTALLMODE=" + Session.Property("REINSTALLMODE");
		if (Session.Property("DISABLE") == 1) cmdLine += " DISABLE=1";
		if (Session.Property("UPDATE") == 1) cmdLine += " UPDATE=1";
		if (Session.Property("TOKEN")) cmdLine += " TOKEN=" + Session.Property("TOKEN");
		if (Session.Property("ROOT_URL")) cmdLine += " ROOT_URL=" + Session.Property("ROOT_URL");
		if (Session.Property("UILevel") == 2) cmdLine += " /q";

		shell.RegWrite("HKLM\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\RunOnce\\InstallEdr", cmdLine);
	}
	catch (err) {
		LogErr(err);
	}
}

function OnStartInstallation() {
	try {
		var shell = new ActiveXObject("WScript.Shell");
		var args = Session.Property("CustomActionData").split("*"); //[OriginalDatabase]*[REINSTALL]*[REINSTALLMODE]*[DISABLE]*[UPDATE]*[UILevel]*[TOKEN]*[ROOT_URL]
		var cmdLine = "msiexec /i " + args[0];
		if (args[1]) cmdLine += " REINSTALL=ALL"; // + args[1];
		if (args[2]) cmdLine += " REINSTALLMODE=" + args[2];
		if (args[3] == 1) cmdLine += " DISABLE=1";
		if (args[4] == 1) cmdLine += " UPDATE=1";
		if (args[6]) cmdLine += " TOKEN=" + args[6];
		if (args[7]) cmdLine += " ROOT_URL=" + args[7];

		if (args[5] == 2) cmdLine += " /q";
		shell.RegWrite("HKLM\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\RunOnce\\InstallEdr", cmdLine);
	}
	catch (err) {
		LogErr(err);
	}

	//MsgBox("OnStartInstallation");

	return 1;
}

function OnSuccessInstallation() {
	// UILEVEL*USER_REPAIR
	var arr = Session.Property("CustomActionData").split("*");
	if (arr[1] == "1") MsgBoxOk("Product is repaired successfully");
	OnEndInstallation();
}

function OnErrorInstallation() {
	// UILEVEL*USER_REPAIR
	var arr = Session.Property("CustomActionData").split("*");
	Log("OnErrorInstallation. UILevel = " + arr[0]);
	if (arr[1] == "1") MsgBox("Product is not repaired");
	if (arr[0] != 2) OnEndInstallation();

	//MsgBox("OnErrorInstallation");
}

function OnEndCancelInstallation() {
	OnEndInstallation();
}

function OnBeforeReboot() {
	OnEndInstallation();
}

function OnEndInstallation(msg) {
	if (msg == null) msg = "Installation finished";
	Log(msg);

	try {
		var shell = new ActiveXObject("WScript.Shell");
		shell.RegDelete("HKLM\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\RunOnce\\InstallEdr");
	}
	catch (err) { }
}

function InstallEdrService() {
	return _ExecSilentCommand(Session.Property("CustomActionData"), "Install edr service");
}

function EnrollEdrService() {
	var args = Session.Property("CustomActionData").split("*");
	var ignore = false;
	if (args[0] == "ALL") ignore = true;
	return _ExecSilentCommand(args[1], "Enroll edr service", ignore);
}

function RollbackInstalledEdrService() {
	return _ExecSilentCommand(Session.Property("CustomActionData"), "Rollback edr service");
}

function StartEdrService() {
	var args = Session.Property("CustomActionData").split("*");
	var ignore = false;
	if (args[0] == "ALL") ignore = true;
	return _ExecSilentCommand(args[1], "Start edr service", ignore);
}

function StopEdrService() {
	_ExecSilentCommand(Session.Property("CustomActionData"), "Stop edr service");
}

function UninstallEdrService() {
	var result = _ExecSilentCommand(Session.Property("CustomActionData"), "Uninstall edr service");
	var service = "edrsvc";
	Log("Start checking service: " + service);
	var runResult = SilentRun("sc query " + service);
	Log("Service control '" + service + "': " + runResult.returnCode);

	if (result == 3 || runResult.returnCode == 0) {
		// delete service manually
		try {
			RegDeleteValue("HKCU\\SOFTWARE", "{D0FF0352-3B0D-4040-A928_038A41AEDB1B}", true); // x64
			RegDeleteValue("HKCU\\SOFTWARE", "{D0FF0352-3B0D-4040-A928_038A41AEDB1B}", false); // x86
			Log("Self protection is disabled");

			_CheckEdrSvc();
			_CheckEdrDrv();
		}
		catch (err) {
			LogErr(err);
		}
	}
}

function DeleteEdrServiceArtefacts() {
	try {
		var fso = new ActiveXObject("Scripting.FileSystemObject");
		fso.DeleteFolder(Session.Property("CustomActionData"), true);
		Log("Edrsvc's artefacts deleted succesfully");
	}
	catch (err) {
		LogErr(err);
	}

	try {
		RegDelete("HKLM\\SOFTWARE\\OpenEdrAgent", true); // x64
		RegDelete("HKLM\\SOFTWARE\\OpenEdrAgent", false); // x86
		Log("Edrsvc's registry artefacts deleted succesfully");
	}
	catch (err) {
		LogErr(err);
	}
}

function _ExecSilentCommand(cmdLine, msg, ignore) {
	Log("Exec silent command: " + cmdLine);
	var runRes = SilentRun(cmdLine);
	Log(msg + ". Result = " + runRes.returnCode + ", output: " + runRes.output + ", ignore: " + ignore);
	
	if (runRes.returnCode == 0 || ignore) return 1;
	return 3;
}