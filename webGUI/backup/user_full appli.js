let serverIP, serverPort, demoIP, demoPort, msModel, serverModel, serverModels, serverOffline;
var ICON_MIXER="!";

function userPong(jsonValue){           // THAT'S THE MANDATORY FUNCTION USER HAS TO IMPLEMENT
    _l("pong!:"+JSON.stringify(jsonValue))
    val = getValue("serverAttempt",jsonValue);
    if (val != undefined) _("serverLink").textContent = val; 
    progress = getValue("syncProgress",jsonValue);
    if (progress != undefined) syncProgress([progress]);
}


function syncProgress(jsonValue){
    val = JSON.stringify(jsonValue[0]);	    
    _("syncProgress").style.width = val+"%";        
    _("syncBgcol").style.backgroundColor = "rgba(0,0,0,0.2)";
}

function mypopup(text){
    alert(text);
}

/* DATA SET GENERATOR FUNCTION */
function getMacroName(){
    return [
      "demo1",
      "demo2",
      "XR16wave",
      "colors"
    ];
}   

// NOT USED
/*
function getDSCh(chSize){
    let ch=[]
    for (var i=0;i<chSize;i++) ch[i]="ch"+i;
    return ch;
}*/

/////////////////////////////////////////
function populateInterfaces(data){
    console.warn(JSON.stringify(data));
    
    emptyElement(serverModels);
    allInterfaces = getValue("interfaces",data);
    allInterfaces.forEach(server => {
        addDropDownEntry(serverModels,server.model,server.model);
    });   
    
    autoConnectModel = _("autoConnectModel");
    emptyElement(autoConnectModel);
    addDropDownEntry(autoConnectModel,-1,i18n.gettext("default"));
    
    allModels = getValue("consoles",data);
    allModels.forEach(model => {
        addDropDownEntry(autoConnectModel,model.consoleId,model.name);
    });      
    
    setModelsDropDown();
}

function setModelsDropDown(){
    serverModels.value = serverModel;       // server
    autoConnectModel.value = msModel;           // ms client    
}

function userConfig(jsonValues){    
    _("testJS").textContent = "IF YOU SEE THIS TEXT IT WORKED :-)";
    setMixer(jsonValues);
    loadJson("interfaces.json",populateInterfaces);
    if (calledFromWizard) _("modelPanel").style.display="none";
}

function reloadBanks(jsonValues){
    if (!calledFromWizard){
        //banks requires to get focus,
        // but before we save which tab is selected to retrieve the same
        _l("banks will be reloaded in 1 second");
        sleep(1000).then(() => { 
            restoreTab = "two";
            if (_("one").checked) restoreTab="one";
            else if (_("three").checked) restoreTab="three";
            
            loadConfig('configBanks'); // as calls are async, this one is triggered after we changed mixer so it refreshes all the strips


            _(restoreTab).checked = true;                                                                        // we howvr have to wait 
        });
        
        saveBt(1);

    }
}

function setPort(model){
    sPort = 10024;
    
    if (model == "MixingStation"){
        _("mixerPort").value = "8080";
        sPort = 3000;
    }else if (model == "X32"){
        sPort = 10023
    }
    _("serverPort").value = sPort;
}


function setMSview(model){
    
    _("serverModel").value = model;
    
    if (model == "MixingStation") msView = "block"; else msView = "none";
    
    els = _("userWizard").getElementsByClassName("mixingStation");
    for (var i=0;i<els.length;i++) els[i].style.display = msView;
    
    setMSConnect(msModel);
}

function autoConnectChange(val){
    setMSConnect(val);
    if (_("autoConnectModel").value != -1) //we only reconnect in case user force the mixer value
    transmitMixer();
}

function setMSConnect(value){
    if (value == -1){
        _("msIPdiv").style.display = "none"; 
    } 
    else{
        _("msIPdiv").style.display = "block";
    }
}

function setMixer(jsonValue){
    demoMixer(jsonValue);
    
    serverIP = getValue("serverIP",jsonValue);
    serverPort = getValue("serverPort",jsonValue);    

    msModel = getValue("autoConnectModel",jsonValue); 
    serverModel = getValue("serverModel",jsonValue);
    serverModels= _("serverModel");
    
    serverOffline = getValue("serverOffline",jsonValue);

    _("serverIP").textContent = serverIP;
    _("mixerPort").value = getValue("mixerPort",jsonValue);
    _("autoConnectIP").value = getValue("autoConnectIP",jsonValue);
    _("userWizard").querySelector('select option[mixerChoice="manual"]').text  = i18n.gettext("manual");
    

    if (getValue("serverConnected",jsonValue) === true){
         _("serverSync").classList.add("sync");
        _("serverLink").textContent=serverModel;
    }else{
         _("serverSync").classList.remove("sync");
	    _("serverLink").textContent = _("serverIP").value +" : "+ _("serverPort").value +" "+_("mixerPort").value;         
    }
    
    setMSview(serverModel);
    
    serverName = getValue("serverName",jsonValue);
    //_("selectMixer").querySelector('option[mixerChoice="discovery"]').text  = "Auto / "+serverName;
    _("selectMixer").querySelector('option[mixerChoice="discovery"]').text  = serverName;

    interfaceState = getValue("mixerConnected",jsonValue);

    _("mixerSync").classList="iconUser surrounded "+interfaceState;
    _("syncProgress").style.width = "0%";        
    _("syncBgcol").style.backgroundColor = "transparent";

    discovery = getValue("serverDiscovery",jsonValue);

    _("connectButton").disabled = false;

    if (interfaceState === "connected"){
        _("serverConnectivityDetails").style.display="none";	

        if (serverIP == demoIP) changeServerIP("demo");
        else changeServerIP(discovery);

        _("waitingMixer").style.display = "none";
	    if (calledFromWizard){
    	  _("secondStepNext").disabled = false;
    	  _("three").disabled = false;
	    }        
	    _("mixerDetails").textContent = getValue("interface",jsonValue); 
	    //_("serverModel").disabled = true;
	    reloadBanks();
    }else{
        //if (_("serverIP").value == "0.0.0.0"){
        if (interfaceState.includes("syncing")) _("connectButton").disabled = true;
        _("waitingMixer").style.display = "block";
        _("waitingMixer").textContent = i18n.gettext("Waiting connection...");
        _("mixerDetails").textContent = i18n.gettext(interfaceState);
        if (serverOffline) changeServerIP("offline");
        else changeServerIP(discovery);        
        //_("serverModel").disabled = false;
	    if (calledFromWizard && !serverOffline){
    	  _("secondStepNext").disabled = true;
    	  _("three").disabled = true;
	    }         
    }
}

function mixerState(jsonValue){
    setMixer(jsonValue); // set mixer will reply by reloadBanks
    setModelsDropDown();
}

function demoMixer(jsonValue){
    demoIP = getValue("demoIP",jsonValue) ;
    demoPort = getValue("demoPort",jsonValue) ;
    
    demo = _("userWizard").querySelector('select option[mixerChoice="demo"]');
    if (demoIP != "") demo.disabled = false;
    else demo.disabled = true;
}


function changeServerIP(mixerChoice){
    _("selectMixer").value = mixerChoice;
    _("selectMixer").querySelector('option[mixerChoice="discovery"]').text  = "auto";
    _("serverConnectivityDetails").style.display="none";		    
	_("modelPanel").style.display="none";		    
	if (mixerChoice == 'offline'){
	    _("modelPanel").style.display="block";
	    _("waitingMixer").textContent = i18n.gettext("Working offline");
        if (calledFromWizard){
          _("firstStepNext").disabled = false;
          _("two").disabled = false;
          _("three").disabled = false;
          _("secondStepNext").disabled = false;
        }  
	}else if (mixerChoice=='demo'){
	    _("serverIPform").value = demoIP;
	    _("serverPort").value = demoPort;
	}else if (mixerChoice == 'manual'){
	    _("serverConnectivityDetails").style.display="block";
	    _("modelPanel").style.display="block";
	    _("serverIPform").value = serverIP;
	    _("serverPort").value = serverPort;
	}else{ // auto
        _("serverIPform").value="0.0.0.0";
        _("serverPort").value = 0;
	}
	
}

function transmitMixer(){
    if (_("selectMixer").value != "manual") connect();        
    
}

function connect(){
    transmit('setMixer',{serverSelected:_("selectMixer").value,serverIP:_("serverIPform").value,serverPort:_("serverPort").value,mixerPort:_("mixerPort").value,serverModel:_("serverModel").value,autoConnectModel:_("autoConnectModel").value,autoConnectIP:_("autoConnectIP").value});    
}