//////////////////////////// gettext.iife.js ////////////////////////////////////////////// 
var i18n = (function () {
    "use strict";
    /*! gettext.js - Guillaume Potier - MIT Licensed */ return function (t) {
        (t = t || {}), this && (this.__version = "1.1.1");
        var r = {
                domain: "messages",
                locale: ("undefined" != typeof document && document.documentElement.getAttribute("lang")) || "en",
                plural_func: function (t) {
                    return { nplurals: 2, plural: 1 != t ? 1 : 0 };
                },
                ctxt_delimiter: String.fromCharCode(4),
            },
            n = function (t) {
                var r = typeof t;
                return "function" === r || ("object" === r && !!t);
            },
            e = {},
            l = t.locale || r.locale,
            a = t.domain || r.domain,
            o = {},
            i = {},
            u = t.ctxt_delimiter || r.ctxt_delimiter;
        t.messages && ((o[a] = {}), (o[a][l] = t.messages)), t.plural_forms && (i[l] = t.plural_forms);
        var s = function (t) {
                var r = arguments;
                return t
                    .replace(/%%/g, "%% ")
                    .replace(/%(\d+)/g, function (t, n) {
                        return r[n];
                    })
                    .replace(/%% /g, "%");
            },
            p = function (t) {
                return -1 !== t.indexOf(u) ? t.split(u)[1] : t;
            },
            c = function (t) {
                for (var r = [t], n = t.lastIndexOf("-"); n > 0; ) (t = t.slice(0, n)), r.push(t), (n = t.lastIndexOf("-"));
                return r;
            },
            f = function (t) {
                var r = (t = t.replace("_", "-")).search(/[.@]/);
                return -1 != r && (t = t.slice(0, r)), t;
            },
            d = function (t) {
                if (!new RegExp("^\\s*nplurals\\s*=\\s*[0-9]+\\s*;\\s*plural\\s*=\\s*(?:\\s|[-\\?\\|&=!<>+*/%:;n0-9_()])+").test(t)) throw new Error(s('The plural form "%1" is not valid', t));
                return new Function("n", "var plural, nplurals; " + t + " return { nplurals: nplurals, plural: (plural === true ? 1 : (plural ? plural : 0)) };");
            },
            g = function (t, r, n) {
                return n.plural_form
                    ? (n.plural_func ? (a = n.plural_func(r)) : (e[l] || (e[l] = d(i[l])), (a = e[l](r))),
                      (void 0 === a.plural || a.plural > a.nplurals || t.length <= a.plural) && (a.plural = 0),
                      s.apply(this, [p(t[a.plural]), r].concat(Array.prototype.slice.call(arguments, 3))))
                    : s.apply(this, [p(t[0])].concat(Array.prototype.slice.call(arguments, 3)));
                var a;
            };
        return {
            strfmt: s,
            expand_locale: c,
            __: function () {
                return this.gettext.apply(this, arguments);
            },
            _n: function () {
                return this.ngettext.apply(this, arguments);
            },
            _p: function () {
                return this.pgettext.apply(this, arguments);
            },
            setMessages: function (t, r, e, l) {
                if (!t || !r || !e) throw new Error("You must provide a domain, a locale and messages");
                if ("string" != typeof t || "string" != typeof r || !n(e)) throw new Error("Invalid arguments");
                return (r = f(r)), l && (i[r] = l), o[t] || (o[t] = {}), (o[t][r] = e), this;
            },
            loadJSON: function (t, e) {
                if ((n(t) || (t = JSON.parse(t)), !t[""] || !t[""].language || !t[""]["plural-forms"])) throw new Error('Wrong JSON, it must have an empty key ("") with "language" and "plural-forms" information');
                var l = t[""];
                return delete t[""], this.setMessages(e || r.domain, l.language, t, l["plural-forms"]);
            },
            setLocale: function (t) {
                return (l = f(t)), this;
            },
            getLocale: function () {
                return l;
            },
            textdomain: function (t) {
                return t ? ((a = t), this) : a;
            },
            gettext: function (t) {
                return this.dcnpgettext.apply(this, [void 0, void 0, t, void 0, void 0].concat(Array.prototype.slice.call(arguments, 1)));
            },
            ngettext: function (t, r, n) {
                return this.dcnpgettext.apply(this, [void 0, void 0, t, r, n].concat(Array.prototype.slice.call(arguments, 3)));
            },
            pgettext: function (t, r) {
                return this.dcnpgettext.apply(this, [void 0, t, r, void 0, void 0].concat(Array.prototype.slice.call(arguments, 2)));
            },
            dcnpgettext: function (t, n, e, i, s) {
                if (((t = t || a), "string" != typeof e)) throw new Error(this.strfmt('Msgid "%1" is not a valid translatable string', e));
                var p,
                    f,
                    d,
                    h = { plural_form: !1 },
                    m = n ? n + u + e : e,
                    y = c(l);
                for (var v in y) if (((d = y[v]), (f = o[t] && o[t][d] && o[t][d][m]), (f = i ? f && "string" != typeof o[t][d][m] : f && "string" == typeof o[t][d][m]))) break;
                return (
                    f ? (p = o[t][d][m]) : ((p = e), (h.plural_func = r.plural_func)),
                    i ? ((h.plural_form = !0), g.apply(this, [f ? p : [e, i], s, h].concat(Array.prototype.slice.call(arguments, 5)))) : g.apply(this, [[p], s, h].concat(Array.prototype.slice.call(arguments, 5)))
                );
            },
        };
    };
})();

///////////////////////////////////////////////////////////////////////////

var intervalJSversion = "0.01c", i18n = new i18n(), allTranslations = ["espanol","deutsch","english","francais","italiano","portugues"], tourSteps= [], guidedTour=null, activeModContainer , configNames, dataRepoJson, demoMode, sourceBtn, translatedAssignements = [], localIP, lastSSID;let messageQueue = [],connectivityPageHasLoaded = false, callBackUponConnection, pingTimeout,pingID,wifiTimeOut,scannedWifiTimeOut,transmitTimeOut=[], userPageHasLoaded = false, shortcutsDefJson,stripSkeletons,waitingCount=0,callFromWizard = false;

var ICON_WIFI=":";
var ICON_ETH="*";
var ICON_FOOTSWITCH="&";
var ICON_ROUTER="!";
var ICON_UPDATE="\"";
var ICON_WIFIETH="'";
var ICON_LINK="F";

const pingInterval = 3000; // Intervalle de ping en millisecondes

var assignementDef = 
    ["no assignement","scroll right","scroll left","increase","decrease","action 1","action 2","action 3","switch layout","change bank","shortcut 1","shortcut 2","shortcut 3"];


// button data
const pressTimes = ["short","medium","hold","double"];
const buttonNames = ["left","right","tap"];

window.onerror = 
  function (msg, source, lineNo, columnNo, error) {
	errMess = "Error: " + msg + 
		  "\nScript: " + source + 
		  "\nLine: " + lineNo + 
		  "\nColumn: " + columnNo + 
		  "\nStackTrace: " + error;
    _e(errMess);
	return true;		
};

function _e(errMess){
	console.log(errMess);
	if (confirm("OUPS...\n Try again?\n\n"+errMess) == true)
		location.reload();	    
}

function _l(object){
    console.log(object);
}

function initIndex(){
    //_l("->initIndex()");
    //_l("initIndex()");

    const params = new URLSearchParams(window.location.search);
    callFromWizard = params.get('wizard'); // verifier si on appelle la pacge en mode wizard

	tourSteps[0] = [
		{elementId: 'tour',triggerNext: 'click'},  
		{elementId: 'two-tab',triggerNext: 'click'},				
	];

	tourSteps[1] = [
		{elementId: 'tour',triggerNext: 'click'},
		{elementId: 'three-tab',triggerNext: 'click'},
	];	

	tourSteps[2] = [
		{elementId: 'tour',triggerNext: 'click'},
	];
	
    if (!isTouchScreen()){
		window.onscroll = function() {
		  var currentScrollPos = window.pageYOffset;
		  if (prevScrollpos > currentScrollPos) {
			document.getElementById("indexTopBar").style.top = "0";
		  } else {
			document.getElementById("indexTopBar").style.top = "-150px";
		  }
		  prevScrollpos = currentScrollPos;
		}
    }else _("path").style.display="none";    // workaround;	
	
	initIconPicker();

    loadFont();    
	_("loadingDiv").classList.add("greyedOut");
	includeHTML();
    
    initAccordions('.fsl-accordion.multiple') ;   

    initBanks();

    _("zoom").value=30;  // initial gridline zoom

    callBackUponConnection = "fullConfig";
	host = window.location.host;
	_("host").textContent = host;
    _("intervalJSversion").textContent = intervalJSversion;	    

	//demoMode = getValue("demoMode",jsonValues);
	if (host == "infrafast.fr"){
        demoMode = true;
        simulSocket();        
	} 
	else{
	    demoMode = undefined;
	    openSocket(host);
	} 

}

function loadJson(filename,processJson){
      
    fetch(filename)   
    	.then(response => {
            return response.json();
    	})
    	.then((data) => {
            processJson(data);            
    	})   
}

function topPendingBar(show){
    //waiting(i18n.gettext("loading fonts"),2);
    elements = document.getElementsByClassName('topProgressBar')
    for (var i = 0; i<elements.length;i++){
      if (show === true) elements[i].style.display = "block";
	  else elements[i].style.display = "none";
    }
    
}


function loadFont(){
    if (document.fonts) {
        // Surveiller l'événement `loading` lorsque le navigateur tente de charger une police
        document.fonts.addEventListener("loading", () => {
            _l("Tentative de chargement d'une police...");
            topPendingBar(true);
        });

        // Surveiller l'événement `loadingdone` lorsque toutes les polices sont chargées ou échouées
        document.fonts.addEventListener("loadingdone", () => {
            _l("Toutes les polices ont terminé leur chargement.");
            topPendingBar(false);
        });

        // Surveiller l'événement `loadingerror` si une erreur se produit lors du chargement
        document.fonts.addEventListener("loadingerror", (event) => {
            _e("Erreur de chargement de la police :", event.error);
            topPendingBar(false);
        });

        /*// Charger une police spécifique pour déclencher l'événement
        document.fonts.load("1em MyCustomFont").then(() => {
            console.log("La police 'MyCustomFont' a été chargée avec succès !");
        }).catch((error) => {
            console.error("Erreur lors du chargement de la police :", error);
        });*/
    } else {
        console.warn("L'API Font Loading n'est pas prise en charge par ce navigateur.");
    }    
}


async function initUser(){
    // we only display the user tab that relates to wizardConfig
    wiz = _("wizardAccordions");
    if (wiz != undefined){
        allAccordions = wiz.getElementsByClassName("fsl-accordion-container");
        for (i=0;i<allAccordions.length;i++) if (allAccordions[i].id != "userWizard") allAccordions[i].style.display="none";
    }
    // call the user code
    postQuery({command:"userConfig", param:""});
    userPageHasLoaded = true;       //may be moved to userCOnfig() function which would indicate the page has also loaded json user conf in addition to HTML
}

function initConnectivity(){
    //_l("->initConnectivity()");
	connectivityPageHasLoaded = true; 
	transmit('retrieve','connectivityConfig');
	transmit('scanWifi','');
	//_l("initConnectivity()->");
}

function _(el) {
  return document.getElementById(el);
}

function wizardEnabled(){
    return _("wizard").checked;
}

/* When the user scrolls down, hide the indexTopBar. When the user scrolls up, show the indexTopBar */
var prevScrollpos = window.pageYOffset;

function enterKeyPressed(event) {
  if (event.keyCode == 13) return true;
  else return false;
}


function refreshTranslations(lang){
	i18n.setLocale(lang);		

    document.querySelectorAll('i18n')
	.forEach((element) => {
	  if (element.firstChild) element.removeChild(element.firstChild); // delete previous text
	  //element.appendChild(document.createTextNode("").innerHTML=i18n.gettext(element.id)));
	  //element.appendChild(document.createElement("text")).innerHTML=i18n.gettext(element.id);
	  element.innerHTML=i18n.gettext(element.id);
	  element.classList.add('clear-animation');
    });	
    
	tourSteps.forEach(function (tourStep,indexi){	//translate tour messages
		tourStep.forEach(function (step, indexj) {
			step.description=i18n.gettext(step.elementId);
			step.title=i18n.gettext(step.elementId+"-title");	
			element = _(step.elementId);
			if (element) element.setAttribute('title', i18n.gettext(step.elementId));
			else _l("missing: "+step.elementId);
		});	    
	});
	
    translateAssignements();

}


function setLang(lang,postProcessCallback=''){
	_("loadingDiv").classList.add("greyedOut");
    
    document.querySelectorAll('i18n')
	.forEach((element) => {
	  element.classList.remove('clear-animation');
    });		
	
	i18JsonMessages = {};

	translationFiles = [lang+".json",lang+"-user.json"];

	translationFiles.forEach(function (item, index) {
        waiting(i18n.gettext("translating"),4);
        //_("i18nloading").classList.add("k-progress-line");
        filename = item.toLowerCase() // lower case because lang files are stored in capital as wizard shows language in UPPERCASE 
        //console.warn("filename:"+filename);
    	fetch(filename)   
    	.then(response => {
    	   return response.json();
    	})
    	.then((data) => {
    		//i18JsonMessages=data; //JSON.stringify(data);
    		Object.assign(i18JsonMessages, data);       // append new values

            //console.log(JSON.stringify(i18JsonMessages));

    		if (index == translationFiles.length-1){
                i18n.setMessages(
                    "messages",
                    lang,
                    i18JsonMessages,
                    "nplurals=2; plural=n>1;"
                );
    		    refreshTranslations(lang);	
        		if (postProcessCallback != ""){
        		  //_l("setLang calling: "+postProcessCallback.name);
        		  postProcessCallback();  
        		} 
                _("loadingDiv").classList.remove("greyedOut");
                showPage();
                //waiting(i18n.gettext("synchro"),5);
               // _("fullLoad").style.display="none";
    		};
    	})
        .catch( err => {
            alert(filename+" not found ");
            _("loadingDiv").classList.remove("greyedOut");
            showPage();
            
        })    	
	});		
}

function dclicChange(id,value){
    transmit('set',{section:'system',id:id,value:value}); 
    setShortcut();
}

function setShortcut(){
    if (_("dClicReactivity").value !=0 ) _("shortcutContainer").style.display="block";
    else _("shortcutContainer").style.display="none";
}

function expandAccordion(expand){
    // expand accordion
    expandables = document.getElementsByClassName("fsl-accordion-container");
    for (i = 0; i < expandables.length; i++){
    	if (expand === true) expandables[i].classList.add('active');	
    	else expandables[i].classList.remove('active');	
    }
}

function exposeOrHideWizard(){
        
    if (wizardEnabled()) expandAccordion(true);
    else expandAccordion(false);

    els = document.querySelectorAll('[wizard="show"]');	
    for (var i=0;i<els.length;i++){
        if (wizardEnabled()) els[i].style.display = "block";
        else els[i].style.display = "none";
    }
	
	els = document.querySelectorAll('[wizard="hide"]');	
    for (var i=0;i<els.length;i++){
        if (wizardEnabled()) els[i].style.display = "none";
        else els[i].style.display = "block";
    }
	
	els = document.querySelectorAll('[index="show"]');	
    for (var i=0;i<els.length;i++) els[i].style.display = "block";
}

function resumeTour(){
	if (tourSteps.length == 0) return;
	document.body.scrollTop = document.documentElement.scrollTop = 0;	
	_('tourButton').checked = getCookie('tourButton')==1? true : false;	
	_("tourNav").className = getCookie('tourButton')==1? '' : 'greyedout';
	if (_("tourButton").checked){ // cookie check
		expandAccordion(true);	// items that are collapsed have to be made visible to properly display the hints
		launchTour();
		
	}
    //showPage();         // this is the right place, do not move elsewhere as this call is done after page has leaded language and in case user changed the language
}

function setTour(){
	setCookie('tourButton', _('tourButton').checked? 1 : 0, 100);
	if (_("tourButton").checked){
		_("tourNav").classList.remove("greyedout");
		resumeTour();
	}else{
		stopTour();
		expandAccordion(false);
		_("tourNav").classList.add("greyedout");
	}
}

function tourNav(direction){
	if (_("tourButton").checked)
		if(direction === 'next'){
			if (!guidedTour.processStep('next')){
			    _("tourButton").checked = false;
			    stopTour();
			}
		}else{			
			if(guidedTour.currentStepIndex > 2) {
				guidedTour.currentStepIndex-=2;
				guidedTour.processStep('next');
			}
		}	
	//}			
}

function stopTour(){
	if (guidedTour!=null){	
		guidedTour.currentStepIndex = tourSteps[wizardIndex].length-1;
		guidedTour.processStep('next');
		guidedTour.processStep('next');
		//guidedTour = null ;
		_("jsa-hint").style.display="none";
		_("jsa-frame").style.display="none";
	}	
}

function setCookie(c_name,value,expiredays) {
	var exdate=new Date()
	exdate.setDate(exdate.getDate()+expiredays)
	document.cookie=c_name+ "=" +escape(value)+((expiredays==null) ? "" : ";expires="+exdate)
}


function getCookie(c_name) {
	if (document.cookie.length>0) {
		c_start=document.cookie.indexOf(c_name + "=")
		if (c_start!=-1) { 
			c_start=c_start + c_name.length+1 
			c_end=document.cookie.indexOf(";",c_start)
			if (c_end==-1) c_end=document.cookie.length
				return unescape(document.cookie.substring(c_start,c_end))
		} 
	}
	return 1
}

function launchTour(){	
	var options = {
		duration: 800,
		styles: {
			frame: {
				customClass: 'tour-frame',	
			},
			hint: {
				customClass: 'tour-hint',
			}
		}
	};	

	if (_("one").checked) wizardIndex = 0;
	else if (_("two").checked) wizardIndex = 1;
	else if (_("three").checked) wizardIndex = 2;
	else if (_("four").checked) wizardIndex = 3;
	stopTour();
	
	/*guidedTour = new StepsJs(tourSteps[wizardIndex], options);  
	_"jsa-hint").style.display="block";
	_("jsa-frame").style.display="block";*/
}

function popup(text){
	alert(text);
}


function includeHTML() {
	var z, i, elmnt, file, xhttp;
	/*loop through a collection of all HTML elements:*/
	z = document.getElementsByTagName("*");
	for (i = 0; i < z.length; i++) {
	  elmnt = z[i];
	  /*search for elements with a certain attribute:*/
	  file = elmnt.getAttribute("w3-include-html");
	  if (file) {
	    //_l("includeHTML('"+file+"')");
		/*make an HTTP request using the attribute value as the file name:*/
		xhttp = new XMLHttpRequest();
        waiting(i18n.gettext("loading files"),3);		
		xhttp.onreadystatechange = function() {
		  if (this.readyState == 4) {
			if (this.status == 200) {
			    elmnt.innerHTML = this.responseText;
			    
    			callBack = elmnt.getAttribute("onloadCallBack");    			
    			if (callBack){
    			    _l("includeHTML calling: "+callBack)
    			    window[callBack](); // function which is called once the HTML page is fully loaded  
    			} 			    
                
			}
			if (this.status == 404) {
				errMsg = "Page not found: "+file;
				elmnt.innerHTML = errMsg;
				_l(errMsg);
			}
			/*remove the attribute, and call this function once more:*/
			elmnt.removeAttribute("w3-include-html");			
			includeHTML();	
		  }
		}      
		xhttp.open("GET", file, true);
		xhttp.send();
		/*exit the function:*/
		return;
	  }
	}
    //setLang(lang,resumeTour);	
    replaceIconTags();
    setsHelper();
    
}

function replaceIconTags() {
    var iconElements = document.querySelectorAll('icon');
    iconElements.forEach(function(icon) {
        iconChar = icon.getAttribute('char');
        font = icon.getAttribute('font');
        if (font == undefined) font = "iconInterval";
        span = document.createElement('label');
        iconID = icon.getAttribute('id');
        span.className = 'surrounded';
        span.classList.add(font);
        span.textContent = eval(iconChar);
        icon.replaceWith(span);
        span.setAttribute("id",iconID);
    });
}


function setsHelper(){
    var helpers = document.querySelectorAll('.help');
    
    helpers.forEach(function(help) {
        //help.style.display = "inline";
        help.onclick = function(event) {
            message = this.getAttribute("message");
            popup(i18n.gettext(message)); 
		}
    });
}

/*function postQuery(jsonObj,callbackF=null){
	if (jsonObj.command != "ping") _l("postQuery: "+JSON.stringify(jsonObj));
    try{
    	fetch('post/',{method: "POST", headers: {'Content-Type': 'application/x-www-form-urlencoded'}, body: JSON.stringify(jsonObj)})
    	//fetch('post',{method: "POST", headers: {'Content-Type': 'application/json'}, body: JSON.stringify(jsonObj)}) // works for esp
    	.then((res) => res.json())
    	.then((data) => {
    		jsonInput = JSON.stringify(data);
    		processIncomingJson(jsonInput);
    		if (callbackF != null) callbackF(); //includeHTML();	
    		//_l(data.answer);		// {"answer":"value"}; 
    	});
    } catch (error) {
        // Gestion des erreurs
        err = "postQuery("+JSON.stringify(jsonObj)+","+callbackF+") : "+error.message;
        _e(err);
    }
}*/


function postQuery(jsonObj, callbackF = null) {
    if (jsonObj.command !== "ping") 
    _l("postQuery: " + JSON.stringify(jsonObj));

    // Utilisation d'une IIFE (Immediately Invoked Function Expression) pour utiliser async/await
    (async () => {
        //try {
            const response = await fetch('post/', {
                method: "POST",
                headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
                body: JSON.stringify(jsonObj)
            });

            // Vérification du statut de la réponse HTTP
            if (!response.ok) {
                throw new Error(`HTTP error! Status: ${response.status}`);
            }

            const data = await response.json(); // Essayer de parser la réponse JSON
            const jsonInput = JSON.stringify(data); // Convertir en string JSON
            processIncomingJson(jsonInput); // Traiter le JSON

            // Appeler la fonction de callback si elle est fournie
            if (callbackF != null) callbackF();

        /*} catch (error) {
            // Gestion des erreurs
            err = "postQuery("+JSON.stringify(jsonObj)+","+callbackF+") : "+error.message;
            _e(err);
        }*/
    })();
}

//https://developer.mozilla.org/fr/docs/Web/API/Document/querySelectorAll
// can update multiple values so we could queue them: updateDataSets([{"/bus02/name":"newval"},{"/bus03/name":"mybus"}])
function updateDataSets(jsonValue){
    jsonObject = jsonValue[0]; // such command only receive one
	//_l("updateDataSets("+JSON.stringify(jsonObject)+")");

    i = 0;
	do {

    	address = Object.keys(jsonObject[i])[0];		
    	value = getValue(address,jsonObject);
	
        // UPDATE ALL EXISTING ADDRESS
        //_l(address+"=>"+value);
    	newaddrr = address.replace(/\//g,"\\\/"); // this is to replace / by escape so from /x/y/z   become \\/x\\/y\\z to be a valid classname selector
        //'[data-attribute="test"]'
        Array.from(document.querySelectorAll('[address="'+newaddrr+'"]')).forEach(function(target) { // a class selector has to start with a dot, followed by his name
        //Array.from(document.querySelectorAll("."+newaddrr)).forEach(function(target) { // a class selector has to start with a dot, followed by his name
    		//_l("updated: "+target.parentNode.parentNode.id+":"+newaddrr+"=>"+value);
    		target.textContent = value;
    	});	
    	
    	// THE UPDATE THE DATASET REPO IT SELF
	    //first we find the generic address name of the address by comparison, 
	    for (addressIndex in dataRepoJson){
	        genericAddress = Object.keys(dataRepoJson[addressIndex])[0];
	        indexMatch=sscanf(address,genericAddress)[0];
	         //we deduct its id array which is the value mbbeded in the address
	         if (indexMatch != null && indexMatch != undefined){
                //_l(genericAddress+" versus "+address+" index match: " +indexMatch+" SETTING VALUE:"+value );	             
                // and we modify the generic data set address with the value at the given matchedIndex
                dataRepoJson[addressIndex][genericAddress][indexMatch-1] = value;
                break;
	         } 
	    }
        
    }while(++i<jsonObject.length);
	
	
}


function waiting(text="⌛",rectangles=0){
/*    if (rectangles==0) _("indicators").style.display="none";
    else{
        rec = _("indicators").querySelectorAll(".rectangle");
        for (var i=0;i<5 && i<rectangles;i++){
            rec[i].classList.add("on");
        }
    }
*/  
    //_l("     \nWAITING\n     ");
    document.body.style.cursor = "wait";
    _("waitingDiv").style.display = "block";
	_("waitingMsg").innerText = text;
	waitingCount++;

}

function initAccordions(elem){
    document.addEventListener('click', function (e) {
        if (e.target.matches(elem+' .fsl-accordion-btn')) {
            if(!e.target.parentElement.classList.contains('active')){
                e.target.parentElement.classList.add('active');
            }else{
                e.target.parentElement.classList.remove('active');
            }
        }
    });
}

function showPage() {
    _("loadingDiv").style.display = "block"; 
    //_l("     \nSHOWPAGE\n     ");
    if(waitingCount--<=0){
        document.body.style.cursor = "default";
        _("waitingDiv").style.display = "none";
        waitingCount = 0;
    }
	//_("loadingDiv").classList.remove("greyedOut");	
}


function cloneComponent(itm, parent, cloneID){
    clone = itm.cloneNode(true);
    clone.id = cloneID;
    _(parent).appendChild(clone);
    return clone;
}



function addDropDownEntry (selectParent,optionValue,optionText,address="") {
    newOption = document.createElement("option");
    
    value = optionValue; // by default or if address manage decimal
    if (address.includes("%s")) value = optionText;
    
    newOption.setAttribute("value",value);
    //if (optionText=="mychannel" || optionText=="mybus") address=optionText;
    if (address!=""){
        address = address.replace(/%02d/i,(optionValue < 10 ? '0' : '') + optionValue );
        address = address.replace(/%d/i,optionValue );
        address = address.replace(/%s/i,value );
        newOption.setAttribute("address",address);
    }
  /*  textContent = document.createTextNode(optionText);
    newOption.appendChild(textContent);		*/
    newOption.innerHTML=optionText;
    if (selectParent!=null) selectParent.appendChild(newOption);
}

// 
// the below function are callback used to feed the strips with their datasets
//
function populatedropdown(dropDown, address, valueArray) {
    //_l("======================dataRepoJson========== "+JSON.stringify(dataRepoJson))
	const jsonValues = dataRepoJson.find(item => address in item);
	
	dataset = jsonValues[address];
	
	// a dataset is either an array of values or  the name of a function to be called that return an array
	
    if (!Array.isArray(dataset)){ //{"/ch/%d/config/name": ["v1","v2","v3"]}
        var callback=""; 
        var size = "";
        if (typeof dataset === "string") {
            //{"/ch/%d/config/name": "getBankNames"}
            callback = dataset;         // dataset = banknames
        } else{ 
            //{"/ch/%d/config/name": {"getDSCh":32}}
            callback = Object.keys(dataset)[0]; // Obtenir la première clé : "getDSCh"
            size = dataset[callback];          // Obtenir la valeur associée : 32        
        }
        //alert(callback+" "+size);
        if (typeof window[callback] === "function") {
            // La fonction existe et peut être appelée
            dataset = window[callback](size);
        } else {
            console.warn(`Dataset callback '${callback}' is not implemented`);
            dataset = [callback] ;
            //dataset = ["a","b"]
        }
    } 

	//jsonValues = getValue(address, dataRepoJson);
	//_l('=>populateDropDown('+dropDown.id+',<'+JSON.stringify(jsonValues)+'>)');
	//emptyElement(item.id);
	
	//addDropDownEntry (selectParent,optionValue,optionText,address="")

    //when getting the data sets, we change all "space" by a "-" because othersise the sscanf function will only take the first word	 //dataset[i].replace(/\s+/g, '~')
    for (var i = 0; i < dataset.length; i++) addDropDownEntry(dropDown,i+1,dataset[i],address);
    
    /*if (valueArray !=null ) selection = valueArray[0]; //  first matched value by sscanf
    else selection = -1;
    
    dropDown.value = selection; // we try to assign the dropdown
    if (dropDown.selectedIndex == -1) dropDown.selectedIndex = 0; // but if it fails, we assign the first option otherwise if no optionas are made, it will fail the transmission*/
    if (valueArray != null) dropDown.value = valueArray[0];

    if (dropDown.selectedIndex == -1) dropDown.classList.add('tour-frame');        
}

function populatefreetext(textElem, address, valueArray){
    //value array is the output of sscanf function which return all values that could match the pattern in stripRender
    if (valueArray == null) return;
    textElem.value="";
    var n = valueArray.length;
    for (var i=0;i<n;i++){
        textElem.value+=valueArray[i];
        if (i<n-1) textElem.value+=" "; // add separator
    }
}

//////////////////////////////////////////////////

function isTouchScreen(){
    if(window.matchMedia("(pointer: coarse)").matches || /Android|webOS|iPhone|iPad|iPod|BlackBerry|edge|IEMobile|Opera Mini/i.test(navigator.userAgent)) return true; else return false;
}

function jsonUpdate(jsonDoc, source) {
    // Parcourir chaque clé de l'objet source
    for (let [key, value] of Object.entries(source)) {
        // Si la clé existe dans l'objet destination et que la valeur est un objet
        if (jsonDoc.hasOwnProperty(key) && typeof jsonDoc[key] === 'object' && !Array.isArray(jsonDoc[key])) {
            // Appeler récursivement la fonction pour mettre à jour l'objet imbriqué
            jsonUpdate(jsonDoc[key], value);
        } else {
            // Si la clé n'existe pas dans l'objet jsonDoc, ou si la valeur n'est pas un objet, mettre à jour la valeur
            jsonDoc[key] = value;
        }
    }
 }
 
function getValue(searchedKey,jsonDoc) {
    for (let key in jsonDoc) {
		//_l("checking key:"+key);		
        if (jsonDoc.hasOwnProperty(key)) {
            if (key === searchedKey) {
				//valeur = JSON.stringify(jsonDoc[key]);
				//valeur = valeur.slice(1, -1); // this removes double quote
				return jsonDoc[key];
            } else if (typeof jsonDoc[key] === 'object') {
				//_l("enter "+key);
                let valeur = getValue(searchedKey,jsonDoc[key]);
                if (valeur !== undefined) {
                    return valeur;
                }
            }
        }
    }
    // Si la clé n'est pas trouvée, vérifions les tableaux
    if (Array.isArray(jsonDoc)) {
        for (let i = 0; i < jsonDoc.length; i++) {
            let valeur = getValue(searchedKey, jsonDoc[i]);
            if (valeur !== undefined) {
                return valeur;
            }
        }
    }
    return undefined;
} 
 

function processIncomingJson(jsonTextInput){
    clearTimeout(transmitTimeOut.shift());
    showPage(); // only show page if we have no more message pending
    //console.log("processIncomingJson :<"+jsonTextInput+">")

    jsonObject = JSON.parse(jsonTextInput);
    
    for (let keyCallback in jsonObject) {
        if (jsonObject.hasOwnProperty(keyCallback)) {
                        
            values = jsonObject[keyCallback];
            //console.log(`Clé: ${keyCallback}, Valeur: ${JSON.stringify(values)}`);
        	if (typeof window[keyCallback] === "function") {		
        		_l("=====> callback: "+keyCallback+"("+JSON.stringify(values)+")");
        		window[keyCallback](values);    //way to call a function call from a variable name
        	}else{
        		console.warn("Callback "+keyCallback+" is not implemented");
        	}            
          
        }
    }    
    
}

function addPopupListener(elem){
	elem.onclick = function(ev) {
		//_l("click"+this);
		charPopup.style.display = "block";	
        _("mainFrame").classList.add("greyedout");
		sourceBtn = this;
	}    
}

function hideIconPicker(){
    // we are done, so hide the popup
    document.getElementById("charPopup").style.display = "none";	
    _("mainFrame").classList.remove("greyedout");    
}

function addIconPickerListener(element){
	allIconPickers = element.querySelectorAll(".charPopupButton");
	for (var i = 0; i < allIconPickers.length; i++) {
        addPopupListener(allIconPickers[i]);
	}
}

function initIconPicker() {
    addIconPickerListener(_('charPopup'))

	// When the user clicks on <span> (x), close the charPopup
	var span = document.getElementsByClassName("closePopup")[0];
	span.onclick = function() {
        hideIconPicker();	  
	}

	// Create the icon picker and add them to the document
	//~example: <button class="charButton" onclick="pasteChar('ì');">ì</button>
	var chars = '!"#$%&\'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~';
	var buttonContainer = document.getElementById("buttonContainer");
	for (var k = 0; k < chars.length; k += 1) {
	//for (var k = 33; k < 127; k += 1) {		
		var node = document.createElement("button");
	    node.style.width="10%";
		var textnode = document.createTextNode(chars[k]);
		node.appendChild(textnode);
		node.className = "charPopupButton";
		node.type = "button";
		node.onclick = function(event) {
			sourceBtn.textContent = event.target.innerHTML;
	  		hideIconPicker();
	  		callback = sourceBtn.getAttribute("callback"); // call the user function associated to the source button
	  		if (callback != undefined) window[callback](sourceBtn); // eval(callback) //
	  		
		}
		buttonContainer.appendChild(node);
		buttonContainer.appendChild(document.createTextNode(" ")); // add a whitespace
	}

}

function stripX(elem){
    alert("stripX: "+elem.classList)
}

function openSocket2(host){	
	srv = 'ws://'+host+':3000';
	_l(">>>>>>>openSocket: "+srv);
	//_l("startSocketEth");
	connect();
	const retryInterval = pingInterval/2; 

	function connect() {
		
		wsocket = new WebSocket(srv);	

	    wsocket.onopen = function(event) {
            _l('WebSocket connection established');
			// pinging in AP mode conflict with wifi AP when a wifi STA connexion could not be establish			
        };	
	

		wsocket.onclose = function(evt) { 			
			waiting(i18n.gettext('Reconnecting...'));
			_("netWorkSyncIndicator").classList.remove("sync");
			_l('WebSocket connection closed. Retrying in '+retryInterval+' ms...');
			// Reconnexion après un délai
			setTimeout(connect, retryInterval);	
		};
		
		wsocket.onerror = function(evt) { 
			_l('wsocketh '+evt.type); 
			//if (evt.type == 'error'){
				wsocket.close();	    
				//location.reload();			
			//}				   
		};
		
		wsocket.onmessage = function(evt) { 
			//_l("wsocketh received:<"+evt.data+">");
			processIncomingJson(evt.data);
		};
	}
}



function openSocket(host){	
	srv = 'ws://'+host+':3000';
	_l(">>>>>>>openSocket: "+srv);
	//_l("startSocketEth");
	const retryInterval = pingInterval/2; // Intervalle de reconnexion en millisecondes doit etre > pingInterval
	connect();

    function startPing() {
        pingID = setInterval(function() {
            if (wsocket.readyState === WebSocket.OPEN) {
                //_l('Sending ping');
                //wsocket.send('{"command":"ping","param":""}');
				wsocket.send('{"command":"ping","param":""}');
                // Définir un timeout pour vérifier la réponse pong
                pingTimeout = setTimeout(function() {
					_l(i18n.gettext('Connection lost'));					
					waiting(i18n.gettext('Waiting connection...'));
					stopPing();
                    wsocket.close();
					_("netWorkSyncIndicator").classList.remove("sync");
                }, pingInterval); // Timeout avant le prochain ping
            }
        }, pingInterval);
    }

    function stopPing() {
		clearInterval(pingID);
        clearTimeout(pingTimeout);
    }

	function connect() {
		
		wsocket = new WebSocket(srv);	

	    wsocket.onopen = function(event) {
            _l('WebSocket connection established');
			// pinging in AP mode conflict with wifi AP when a wifi STA connexion could not be established
			//sleep(pingInterval).then(() => { startPing();}); // we start pinging 3 seconds after socket connection was established
			
			// Envoyer tous les messages dans la file d'attente
			while (messageQueue.length > 0) {
				//showPage();
				const message = messageQueue.shift();
				wsocket.send(message);
				_l('Message sent from queue:'+message);
			}			
			
			loadConfig(callBackUponConnection);
			
        };	
	
		/*wsocket.addEventListener('open', (event) => {
			_l('WebSocket is open now.');
			// Envoyer tous les messages dans la file d'attente
			while (messageQueue.length > 0) {
				showPage();
				const message = messageQueue.shift();
				wsocket.send(message);
				_l('Message sent from queue:', message);
			}
		});	*/

		wsocket.onclose = function(evt) { 			
			stopPing();			
			waiting(i18n.gettext('Reconnecting...'));
			_("netWorkSyncIndicator").classList.remove("sync");
			_l('WebSocket connection closed. Retrying in '+retryInterval+' ms...');
			// Reconnexion après un délai
			setTimeout(connect, retryInterval);	
		};
		
		wsocket.onerror = function(evt) { 
			_l('wsocketh '+evt.type); 
			//if (evt.type == 'error'){
				wsocket.close();	    
				//location.reload();			
			//}				   
		};
		
		wsocket.onmessage = function(evt) { 
			//_l("wsocketh received:<"+evt.data+">");
			processIncomingJson(evt.data);
		};
	}
}

function sleep(ms) {
  return new Promise(resolve => setTimeout(resolve, ms));
}

function sendMessage(message) {
	if (wsocket.readyState === WebSocket.OPEN) {
		wsocket.send(message);
	} else {
		messageQueue.push(message);
		_l('queuedSocket:'+message);
	}
}

async function transmit(command, param, transmitTO=500){
	jsonCommand = {command:command,param:param};
	jsonString = JSON.stringify(jsonCommand);
	//_l("transmit: "+jsonString);			
    //waiting();
    if (demoMode == undefined) sendMessage(jsonString); // for real 
	else sleep(Math.floor(Math.random() * 100)).then(() => { postQuery({command:command, param:param}); }); // simulated //2000
	//else postQuery({command:command, param:param});
    
    fID = setTimeout(function() {
            msg = i18n.gettext('Processing...');
            //msg += "<br>"+command+" "+param;
            waiting(msg);
            //waiting();
        }, transmitTO);
    transmitTimeOut.push(fID);
}

function disableSaveIcon(){
	_("saveBt").disabled = true;
	_("saveBt").classList.remove('blink_me');	
}

function enableOrDisableLoadDelButton(){
    ifConfExists = configNames.includes(_('configDropdown').value);
    _("loadBt").disabled = !ifConfExists;
    _("deleteBt").disabled = !ifConfExists;
    if (_('configDropdown').options.length<=1) _("deleteBt").disabled=true;
}

function configDropDownChange(){
    //_l("updated dropdown, current value="+_('configDropdown').value);
    _("saveBt").disabled = false;	
    enableOrDisableLoadDelButton();
}

function emptyElement(myNode){
    if (myNode!=null) while (myNode.firstChild) {
        myNode.removeChild(myNode.lastChild);
    }	
}

function refreshButtonLang(){
	translateAssignements();
	const allSelects = _("buttonAssignementPanel").querySelectorAll('select');
	allSelects.forEach((select) => {	
	  for (i = 0; i < select.options.length; i++)
	   select.options[i].innerHTML = translatedAssignements[select.options[i].value-1];	
	});	
}

 
function assignButtons(){
    // clear the assignement templates options
    emptyElement(_("assignementTPL"));
    // populate the template with the possible values
    for (var i = 0; i < translatedAssignements.length; i++){
        addDropDownEntry(assignementTPL,i,translatedAssignements[i]);
        //_l("-------> "+translatedAssignements[i])
    }

    // then populate assignement table with templates
    pressTimes.forEach(function (b, index){	
    	for (i = 0; i<buttonNames.length;i++){
    		target = pressTimes[index]+"-"+buttonNames[i];
    		_(target).innerHTML = "";
    		cloneComponent(_("assignementTPL"), target, target+"-dropDown");
    		_(target+"-dropDown").style.display="inline";
    		//alert("cloneComponent("+_("assignementTPL")+","+target+","+target+"-dropDown)");
    	}
    });
    
    //buttonsAssignements = JSON.parse(_("pressTimes").value);
    //and apply the user setting values to all drop down from table
    buttonNames.forEach(function (b, index){  	
    	for (i = 0; i<pressTimes.length;i++){
    		el = pressTimes[i]+"-"+b+"-dropDown";
    		//alert(el+"="+buttonsAssignements[b][i]); 
    		_(el).value = buttonsAssignements[b][i];
    	}			
    });   
    // filter all entries which are already used  (exclusive assignement)
    allSelects = _("buttonAssignementPanel").querySelectorAll('select');
    allSelects.forEach((select) => {
    	//handleMutuallyExclusiveSelect(select.id);
    });    
}

function networkPending(){
	_("attemptConnecting").style.display="block";	            
	_("waitingNetConnection").style.display="none";
    _("netWorkSyncIndicator").classList.remove("sync");
    _("netWorkSyncIndicator").classList.add("pendingConnect");
}

function saveBt(state){
    _l("saveBt("+state+")");
    if (state == 1){
    	_("saveBt").disabled = false;
    	_("saveBt").className="blink_me";	
    }else{
    	_("saveBt").disabled = true;
    	_("saveBt").classList.remove('blink_me');		
    }    
}

function enableScanButton(){
    _('wifi').disabled = false;
    _('scanButton').disabled = false;
}

////////////////////// Json Input function callback by key which are called by startCommunication
function undefinedCallBack(jsonValue){
	_l("undefinedCallBack");
}

function modalMessage(jsonValue){
    msg = JSON.stringify(jsonValue[0]).replace(/"/g, '');
    alert(i18n.gettext(msg));
}

function commandError(jsonValue){
    modalMessage(jsonValue);
    loadConfig(callBackUponConnection);
}

function wifiConnectTimeout(jsonValue){
    timeOutDuration = JSON.stringify(jsonValue[0]);	
    timeOutDuration = parseInt(timeOutDuration.slice(1, -1));   // convert to int and remove double quote
    waiting("Connecting WiFi");
    networkPending();
    disableWizardStep2();    
    wifiTimeOut = setTimeout(function() {
		transmit('stopWifi','');
		_l(i18n.gettext('Waiting Wifi'));					
        showPage();
        /*_('pwd').value = "";    // reseting pwd
        _('wifiNameDiv').value = "";*/
        setLink(false,false); // no network, no cable
        alert(i18n.gettext("Incorrect WiFi credentials or WiFi not available"));
        
    }, timeOutDuration); 
    _("networkIcon").textContent = ICON_LINK;
}

function pong(jsonValue){
	// Réponse pong reçue, la connexion est active
	//_l('Pong received');
	clearTimeout(pingTimeout);		
	if (userPageHasLoaded){
	    if (typeof userPong === 'function') { userPong(jsonValue); }  // we have a pong user fonction
	}
}

function wifiScanStart(jsonValue){
	_('updateIcon').classList.add('rotate-text');
    _('scanButton').disabled = true;

    scannedWifiTimeOut = setTimeout(function() {
        alert("Scan WiFi timed out");
        scannedWifi([]);
    }, 13000);     
}

function sendWifiCredentials(){
    if (_("networkIcon").textContent == ICON_ETH) transmit('setWifi',{ssid:_('ssid').value,password:_('pwd').value});
    else{ // are we connected in wifi?
        if (_("netWorkSyncIndicator").classList.contains("sync")){
            alert(i18n.gettext("warningWiFiDisconnected"))// warn user as this will cut the current connexion
            transmit('setWifi',{ssid:_('ssid').value,password:_('pwd').value});    
        }else transmit('setWifi',{ssid:_('ssid').value,password:_('pwd').value});
    }     
}

function setSaveButton(jsonValue){
	val = JSON.stringify(jsonValue[0]);	
	val = val.slice(1, -1); // this removes double quote
	_l("val: <"+val+">");
	saveBt(parseInt(val));
}

function scannedWifi(jsonValue){
    clearTimeout(scannedWifiTimeOut);    
	_('updateIcon').classList.remove('rotate-text');	
	emptyElement(_("wifi"));
	//_l("ScannedWifi("+JSON.stringify(jsonValue)+")");
    other = i18n.gettext('other');
	addDropDownEntry( _("wifi"),"other",other);
	_("wifi").value = other;

	jsonValue.forEach(value => {
			addDropDownEntry(_("wifi"),value,value);
	});

	if (containsOption(_("wifi"),lastSSID)){
        _("wifi").value = lastSSID;
        _("wifiNameDiv").style.display="none";
        
	} 
	else{
	    _("wifiNameDiv").style.display="inline";
	    _("wifi").value="other";
	}
    _("ssid").value = lastSSID;

    enableScanButton();
}


function setWifiName(){
    if ( _('wifi').value =='other'){ // in case translation failed
        /*_("ssid").disabled = false;
        _('ssid').style.background="#3498db";*/
        _("wifiNameDiv").style.display="inline";        
        _('ssid').classList.add('tour-frame'); _('ssid').focus();
        _('pwd').classList.remove('tour-frame');
    }else{
        /*_("ssid").disabled = true;
        _('ssid').style.background="grey";*/
        _("wifiNameDiv").style.display="none";
        _("ssid").value = _('wifi').value;
        _('pwd').classList.add('tour-frame'); _('pwd').focus();
    }
    
}

//////////////////////////////////////////////////////////////////////////////////////////


function volatile(jsonValues){
    langs = getValue("langs",jsonValues);
    emptyElement(_("translationDropDown"));
    for (var i = 0; i < langs.length; i++) addDropDownEntry(_("translationDropDown"),langs[i],langs[i]);
    _("wizard").checked = getValue("wizard",jsonValues) || callFromWizard;
    exposeOrHideWizard();
    _("wizardMessage").textContent = i18n.gettext("Step 1: connect the network");
}

function connectivityConfig(jsonValues){
    if (connectivityPageHasLoaded){

		if (getValue("useNetwork",jsonValues) != false){
			
			_("useNetwork").style.display="block";
			
			_("localIP").textContent = getValue("localIP",jsonValues);
			_("gatewayIP").textContent = getValue("gatewayIP",jsonValues);	

			_("netWorkSyncIndicator").classList.remove("sync");
			_("netWorkSyncIndicator").classList.remove("pendingConnect");
			
            lastSSID = getValue("lastSSID",jsonValues);			
			netState = getValue("networkState",jsonValues);
			cable = getValue("cable",jsonValues);

            setLink(netState,cable);
            
            enableScanButton();
            
		}else{
			if (wizardEnabled()) _("firstStepNext").disabled = false;
		}
		_("panel1loadingProgress").style.display="none";    
		
    }

}

function togglePasswordVisibility() {
    const passwordField = document.getElementById("pwd");
    const togglePassword = document.getElementById("togglePassword");

    // Basculer le type entre "password" et "text"
    const type = passwordField.type === "password" ? "text" : "password";
    passwordField.type = type;

}

function containsOption(selectElement, optionValue) {
    for (let i = 0; i < selectElement.options.length; i++) {
        if (selectElement.options[i].value === optionValue) {
            return true;
        }
    }
    return false;
}


function setLink(netState,cable){
    buttonText = i18n.gettext("connect");
	if (cable != false){
	    _("networkIcon").textContent = ICON_ETH;
	    buttonText = i18n.gettext("save");
	} 
	else if (netState == false && !cable ) _("networkIcon").textContent = ICON_WIFIETH;
	else _("networkIcon").textContent = ICON_WIFI;

    _("wifiButton").innerText = buttonText;

    //if (cable != false) _("collapsible").checked = false;
    if (netState) _("collapsible").checked = false;     //we only show wifi setting if there is no connection
	else _("collapsible").checked = true;
	
	if (netState != false){
		
		_("waitingNetConnection").style.display="none";
		_("attemptConnecting").style.display="none";
		_("netWorkSyncIndicator").classList.add("sync");    	    
		clearTimeout(wifiTimeOut);    
		_l("CLEARED WIFI TIMEOUT");
		
	}else{
		if (cable){
			networkPending();
		} 
		else{
			_("waitingNetConnection").style.display="block";
			_("netWorkSyncIndicator").classList.remove("sync");
			_("netWorkSyncIndicator").classList.remove("pendingConnect");
			_("attemptConnecting").style.display="none";
		} 
		disableWizardStep2();
	}
    
}

function loadConfig(whichConfig){
    postQuery({command:"retrieve", param:whichConfig});     
}

function simulSocket(callBack){
    //sleep(0).then(() => { transmit('retrieve','connectivityConfig');}); // when socket is connected, it immediatley send the 
    loadConfig(callBackUponConnection);
    setInterval(function() {
        postQuery({command:'ping', param:''});
    }, pingInterval);    
}

function skeletons(jsonValues){
    stripSkeletons = jsonValues;
    skelNameDropDown = _("stripMaskTPL").querySelector(".skelName");    
    populateSkeletonsDropDown(stripSkeletons,skelNameDropDown);    
}

function shortcutsDef(jsonValues){
    shortcutsDefJson = jsonValues;
}

function system(jsonValues){
	configNames = getValue("configNames",jsonValues);
	//assignementDef = getValue("assignementsDef",jsonValues); // variable is stored locally (at the moment)...
	buttonsAssignements = getValue("buttonsAssignements",jsonValues);
    shortcuts = getValue("shortcuts",jsonValues);    
    _("apName").value = getValue("apName",jsonValues);        
	//apply system settings
	_("pushDelay").value = getValue("pushDelay",jsonValues);
	_("dClicReactivity").value = getValue("dClicReactivity",jsonValues);
	setShortcut();
	_("faderSpeed").value = getValue("faderSpeed",jsonValues);
    feedShortcuts();
    lang = getValue("lang",jsonValues);    
    _("translationDropDown").value = lang;    
    setLang(lang,resumeTour);
    ///////////// ACTION TO BE LEFT AT THE END OF THE INIT ////////////////////////////////////////////
    //zippedHello();	
    //document.getElementById("one").click();    
    //setLang(lang,resumeTour);	    
	//_l("end initIndex()");	
}

function feedShortcuts(){

    // STILL TO BE DONE : ADD THE ADDRESS NAME (WHICH INDICATE WHAT THE PROPERTY IS USED FOR)
    // DATASETS OF SHORTCUT HAVE TO BE MANAGED DYNAMICALLY, FOR INSTANCE IF YOU CHANGE A MODULE NAME OR THE STRIPS, IT CALL UPDATEDATASET
    // ADD THE FUNCTION "ONCHANGE" in THE shortcutID DROPDOWN SO IT CALLS AGAIN buildSkeleton WHEN CHANGING THE SHORTCUT

    for (shortcutID in shortcuts){
        shortcut = shortcuts[shortcutID];
        //_l(shortcutID+" => "+JSON.stringify(shortcut))    
        populateSkeletonsDropDown(shortcutsDefJson,_(shortcutID));

        // restore the shortcut selected by user
        shortcutType = shortcut["type"]
        _(shortcutID).value = shortcutType;        

        updateShortcutProps(shortcutID);
    }
    
    //_l("shortcutsDefJson => "+JSON.stringify(shortcutsDefJson))
}

//buildSkeleton(skelName,skeletons,targetEl,propertiesValues)

function updateShortcutProps(shortcutID){
    shortcutType = _(shortcutID).value;
    
    // destination dropdown
    destination = _(shortcutID+"Props");
    shortcutProperties = shortcut["properties"];
    // populate the shortcut properties
    buildSkeleton(shortcutType,shortcutsDefJson,destination,shortcutProperties,"shortCutProperty");

}

function sprintf(theString, argumentArray) {
    //only support %s
    var regex = /%s/;
    var _r=function(p,c){return p.replace(regex,c);}
    return argumentArray.reduce(_r, theString);
}



function transmitShortcut(){

    // Objet JSON initial
    const jsonValues = {
    };
    

    //function call => shortcut onchange()
    for (var i=1;i<4;i++){
        
        id = "shortcut"+i;
        type = _(id).value;
        props = _(id+"Props").querySelectorAll(".shortCutProperty");
        jsonValues[id] = {type:type,properties:forgeProperties(props)}
    
    }

    transmit('set',{section:'system',id:'shortcuts',value:jsonValues})    
    
}

function translateAssignements(){
	for (i=0;i<assignementDef.length;i++) translatedAssignements[i]=i18n.gettext(assignementDef[i]); 
}


function sscanf(e,t){let r=[],a=/\S/,n=arguments,c,s=function(e){let a=t.slice(e).match(/%[cdeEufgosxX]/g);if(a){let n=a.length;for(;n--;)r.push(null)}return i()};var i=function(){if(2===n.length)return r;for(var e=0;e<r.length;++e)n[e+2].value=r[e];return e};let l=function(t,a,n){if(o){let s=e.slice(t),i=h?s.substr(0,h):s,l=a.exec(i),u=void 0!==c?c:r.length,f=r[u]=l?n?n.apply(null,l):l[0]:null;if(null===f)throw Error("No match in string");return t+l[0].length}return t};if(arguments.length<2)throw Error("Not enough arguments passed to sscanf");for(let u=0,f=0;u<t.length;u++){var h=0,o=!0;if("%"===t.charAt(u)){if("%"===t.charAt(u+1)){if("%"===e.charAt(f)){++u,++f;continue}return s(u+2)}let $=/^(?:(\d+)\$)?(\*)?(\d*)([hlL]?)/g,d=$.exec(t.slice(u+1)),_=c;if(_&&void 0===d[1]){let g="All groups in sscanf() must be expressed as numeric if ";throw Error(g+="any have already been used")}c=d[1]?parseInt(d[1],10)-1:void 0,o=!d[2],h=parseInt(d[3],10);let b=d[4];if(u+=$.lastIndex,b)switch(b){case"h":case"l":case"L":break;default:throw Error("Unexpected size specifier in sscanf()!")}try{switch(t.charAt(u+1)){case"F":case"g":case"G":case"b":break;case"i":var k=/([+-])?(?:(?:0x([\da-fA-F]+))|(?:0([0-7]+))|(\d+))/;f=l(f,k,function(e,t,r,a,n){return r?parseInt(e,16):a?parseInt(e,8):parseInt(e,10)});break;case"n":r[void 0!==c?c:r.length-1]=f;break;case"c":f=l(f,RegExp(".{1,"+(h||1)+"}"));break;case"D":case"d":f=l(f,/([+-])?(?:0*)(\d+)/,function(e,t,r){let a=parseInt((t||"")+r,10);return a<0?a<-2147483648?-2147483648:a:a<2147483647?a:2147483647});break;case"f":case"E":case"e":f=l(f,/([+-])?(?:0*)(\d*\.?\d*(?:[eE]?\d+)?)/,function(e,t,r){return"."===r?null:parseFloat((t||"")+r)});break;case"u":f=l(f,/([+-])?(?:0*)(\d+)/,function(e,t,r){let a=parseInt(r,10);return"-"===t?4294967296-a:a<4294967295?a:4294967295});break;case"o":f=l(f,/([+-])?(?:0([0-7]+))/,function(e,t,r){return parseInt(e,8)});break;case"s":f=l(f,/\S+/);break;case"X":case"x":f=l(f,/([+-])?(?:(?:0x)?([\da-fA-F]+))/,function(e,t,r){return parseInt(e,16)});break;case"":throw Error("Missing character after percent mark in sscanf() format argument");default:throw Error("Unrecognized character after percent mark in sscanf() format argument")}}catch(m){if("No match in string"===m)return s(u+2)}++u}else if(t.charAt(u)!==e.charAt(f)){if(a.lastIndex=0,a.test(e.charAt(f))||""===e.charAt(f))return s(u+1);e=e.slice(0,f)+e.slice(f+1),u--}else f++}return i()}

/////////////////////////////////// BANKS ////////////////////////////////

let draggedElement,dropOriginalInputs,previousTarget = null,radioButtons,dropOrigin = "move",menuDrop,menuClickHandler,widgetDropMenuIsOpen,activeBank,jsonStripPositions,posBeforeMove, idBeforeMove; 
let newWidgetMenu = {
    "event": null,
    "1x4": false,
    "4x1": false,
    "2x2": false,
    "4x4": false
};

//let canvasAnimation


const maxBankNameL=12;

function datasets(jsonValues){
    dataRepoJson = jsonValues;
}

function banks(jsonValues){
    banksJson = jsonValues;  
    feedBanks(banksJson);    
}

function initBanks(){
    menuDrop = _('menu');

    menuClickHandler = function (e) {
        event = newWidgetMenu["event"];
        grid = event.target.parentElement;        
        
        if (e.target.classList.contains("widget-1x4")) createWidget(".widget-1x4",event,grid,"new");
        else if (e.target.classList.contains("widget-4x1")) createWidget(".widget-4x1",event,grid,"new");
        else if (e.target.classList.contains("widget-2x2")) createWidget(".widget-2x2",event,grid,"new");
        else if (e.target.classList.contains("widget-4x4")) createWidget(".widget-4x4",event,grid,"new");
        
        hideWidgetDropMenu();

    };    
    
    // Gestion du début du drag pour les widgets
    document.addEventListener('dragstart', (event) => {
        if (event.target.classList.contains('widget')) {
            if (event.target.parentElement.closest('.widgets-container')) dropOrigin = "new";
            if (dropOrigin == "new" || dropOrigin == "duplicate"){   
                draggedElement = duplicateWidget(event.target);
            } else {
                draggedElement = event.target;
                // save the coordinate origin to inform former/new position
                posBeforeMove = calculateXY(event.clientX,event.clientY,grid);
                idBeforeMove = parseInt(draggedElement.querySelector(".uniqueID").textContent);
            }
            event.dataTransfer.setData('text/plain', 'widget');
            //event.dataTransfer.effectAllowed = dropOrigin ? 'copy' : 'move';
            
            
        }
    });    
    
    if (isTouchScreen()) _("stripGroup").style.display='none';    
}


function feedBanks(banksJson){
	// we received the banks defition

    // empty bank headers
    emptyElement(_("flexibleGridZone"));
    // reset navigator (new icon)
    _("bankNavigator").innerHTML = '<div class="newBank"><button class="newBankButton bankIcon textShad" onclick="newEmptyBank()">&#43;</button></div>';

    //we must put the focus on the banks tab otherwise the calculation of coordinates on the cells will fail because grids are not visible/shown
    // but before we save which tab is selected to retrieve the same
    restoreTab = "three";
    if (_("one").checked) restoreTab="one";
    else if (_("three").checked) restoreTab="three";
    
    showPage();
    _("three").checked = true;

    for (var bankName  in banksJson){
        bankDefinition = banksJson[bankName];
        //create the grids
        
        buildBankContainer(bankName,bankDefinition);
        feedBankWidgets(bankName,bankDefinition);
    }
    
    document.querySelector('.draggable').addEventListener('dragend', function(event) {
        clearTargetCell(previousTarget);
    });
    
    //masquer les strips
    const strips = document.querySelectorAll('.stripMask');
    strips.forEach(strip => {
        hideStrip(strip);
    });
    
    selectBank(bankName); // select last bank by default    
    zoomGrid(_("zoom").value);
    
    _(restoreTab).checked = true;
}

function feedBankWidgets(bankname,jsonContent){
    for (i=0;i<jsonContent.strips.length;i++){
        widgetDefinition = [{bank:bankname,strip:jsonContent.strips[i]}]
        restoreStrip(widgetDefinition);
        //_l("feed: "+bankname+"("+x+","+y+")/"+win+" "+size+" "+stripSkelName);        
    }  
}    

function selectBank(bankName){
    _l("BANKNAME: "+bankName);
    hideAllBanks();
    _("connectionCanvas").style.visibility="visible";
    bankSelectors = document.querySelectorAll('.bankSelector');
    bankSelectors.forEach(bankSelector => {
        bankSelector.classList.remove("bankSelected");
        bankNameEl = bankSelector.querySelector(".bankName")
        bankNameEl.classList.remove("lightOn");
        bankNameEl.classList.remove("lightOff");        
        let col = "lightOff";
        if (bankNameEl.value == bankName ){
            bankSelector.classList.add("bankSelected");
            col="lightOn";
            activeBank = bankName;
        }
        bankNameEl.classList.add(col);
    }); 
    
    bank = _(bankName);    
    bank.style.display="flex";    
    grids = bank.querySelectorAll('.gridParent');
    grids.forEach(grid => {
        parseGrid(grid,identifyGridItem);   // now that the grid layout has changed, we identify again the cells
/*        grid.classList.toggle("flyIn");
        grid.classList.remove("flyOut");*/
    }); 
    drawCurve();
}

function hideAllBanks(){
    allBanks = document.querySelectorAll('.bank');
    allBanks.forEach(bank => {
        bank.style.display="none";

        /*grids = bank.querySelectorAll('.gridParent');         // fade out transition
        grids.forEach(grid => {
            grid.classList.add("flyOut");
            grid.classList.remove("flyIn");
        });*/


    });    
}

function createWidget(shape,event,grid, from, stripValues=null){
    dropOrigin = from;
    widget = duplicateWidget(_("widgets-repo").querySelector(shape));
    const { x, y } = calculateXY(event.clientX,event.clientY,grid);
    // DETERMINE THE WIDGET SIZE AND ID BASED ON THE SHAPE CLASS
    const {sizeX, sizeY } = getElementSize(widget);
    dropWidget(widget,grid,x,y,sizeX,sizeY,stripValues);    
}

function scanGridItem(clientX, clientY, grid){
    const { x, y } = calculateXY(clientX, clientY, grid);

    // records data used by the menu to manage appropriate items
    //freeCell = 0;
    freeCell = 0;
    newWidgetMenu["1x4"]=false;
    newWidgetMenu["4x1"]=false;
    newWidgetMenu["2x2"]=false;
    newWidgetMenu["4x4"]=false;

    if (isWithinGridBounds(x, y, 1, 4) ){
        if (!checkCollision(x, y, 1, 4, grid)){ // check collision is only checked if it's inbound.
            newWidgetMenu["1x4"] = true;    // memorize that this widget can be dropped by the menu
            freeCell += 1;
        }
    }

    
    if (isWithinGridBounds(x, y, 4, 1)){ 
        if (!checkCollision(x, y, 4, 1, grid)){ 
            newWidgetMenu["4x1"] = true;
            freeCell += 1;
        }
    }

    if (isWithinGridBounds(x, y, 2, 2)){
        if (!checkCollision(x, y, 2, 2, grid)){
            newWidgetMenu["2x2"] = true;
            freeCell += 1;
        }
    }

    if (isWithinGridBounds(x, y, 4, 4)){
        if (!checkCollision(x, y, 4, 4, grid)){        
            newWidgetMenu["4x4"] = true;
            freeCell += 1;
        }
    }

    
    //_l(grid.id+"("+x+","+y+") return "+freeCell );
    
    return freeCell;
}


function showWidgetDropMenu(grid){
    //https://phuoc.ng/collection/html-dom/show-a-custom-context-menu-at-clicked-position/
    // Show the menu
    const gridRect = grid.parentElement.getBoundingClientRect();
    
    var title = _("menu").querySelector('.container__divider');
    title.insertAdjacentHTML('afterend', '<div id="widgetsList"></div>');
    
    menuRoot = _("widgetsList");
    menuItems = "";
    
    if (newWidgetMenu["1x4"]) menuItems = '<li class="container__item widget-1x4">vertical</li>';
    if (newWidgetMenu["4x1"]) menuItems += '<li class="container__item widget-4x1">horizontal</li>';
    if (newWidgetMenu["2x2"]) menuItems += '<li class="container__item widget-2x2">square</li>';
    if (newWidgetMenu["4x4"]) menuItems += '<li class="container__item widget-4x4">full size</li>';

    menuRoot.innerHTML = menuItems;

    menuDrop.classList.remove('container__menu--hidden');
    const menuRect = menuDrop.getBoundingClientRect();

    // Set the position for menu
    y = gridRect.height/2 + gridRect.top    // get Y center of the grid
    y -= menuRect.height;                   // deduct the menu height

    x = gridRect.width/2 + gridRect.left    // get Y center of the grid
    x -= menuRect.width/2;                   // deduct the menu height

    menuDrop.style.top = y + 'px';
    menuDrop.style.left = x + 'px';    

    menuDrop.addEventListener('click', menuClickHandler);     
    widgetDropMenuIsOpen = true;
}


function hideWidgetDropMenu(){
    //_l("   hideWidgetDropMenu   ")
    menuDrop.classList.add('container__menu--hidden');
    menuDrop.removeEventListener('click', menuClickHandler);  
    widgetDropMenuIsOpen = false;
    _('widgetsList').remove();
    //clearTargetCell(previousTarget);
}


function duplicateWidget(element){
    copy = element.cloneNode(true); // Dupliquer le widget
    copy.classList.add('dropped');
    copy.style.position = 'absolute';
    // sauvegarder les valeurs des inputs
    dropOriginalInputs = element.querySelectorAll('input, select, textarea');  
    return copy;
}

function dropWidget(draggedElement,gridParent,x,y,sizeX, sizeY, stripValues=null){
    let elementToAdd;

    if (dropOrigin == "move") {
        // mouvement d'une grille a l'autre
        sourceGrid = draggedElement.parentElement;
        if (sourceGrid !== gridParent) {
            // ajouter les cellules qui combleront le trou laissé par le widget nue fois supprimé
            addCells(draggedElement);
            trashStrip(draggedElement);
            //la suppression du widget est faite par l'operation setStrip qui arrive après

            // Ajouter l'élément à la nouvelle grille
            elementToAdd = draggedElement;
            updateGrid(gridParent,elementToAdd);
            parseGrid(sourceGrid,identifyGridItem);   // now that the grid layout has changed, we identify again the cells in the old  grid
            dropOrigin = "duplicate"; // => to recreate the widget later on
            
        } else {
            //mouvement dans une meme grille
            elementToAdd = draggedElement;
        }

    } else {
        // check if the widget has an attached strip, otherwise create it and attach it below the widget element class
        if (!draggedElement.querySelector(".stripMask")){
            
            strip = _("stripMaskTPL").cloneNode(true); 
            strip.removeAttribute('id');
            //_l(draggedElement.classList)
            draggedElement.appendChild(strip);
            widgetMask = _("widgetMaskTPL").cloneNode(true);
            widgetMask.removeAttribute("id");
            draggedElement.appendChild(widgetMask);
            
            /*strip.insertAdjacentHTML('afterend', '<div class="widgetBottomLabel"></div>');
            strip.insertAdjacentHTML('afterend', '<div class="widgetTopLabel"></div>');
            strip.insertAdjacentHTML('afterend', '<label class="adaptativeIcon" style="font-size: 3vw">5</label>')*/
            
            renderStrip(strip, stripValues);
            // we had to create the strip so we display it in case it's a manually dropped one so user can configure it straight after drop
            if (dropOrigin == "new"){
                showStrip(strip);  
                isModified(strip);
            }
            
            const {sizeX, sizeY}= getElementSize(draggedElement);
            draggedElement.setAttribute("size",sizeX+"x"+sizeY);
        }
        
        elementToAdd = draggedElement;
        //elementToAdd.dataSet.size = size;
        // coller manuellement les valeurs des inputs
        const clonedInputs = elementToAdd.querySelectorAll('input, select, textarea');
        
        dropOriginalInputs.forEach((input, index) => {
            // dupliaue le contenu des proprietés
            if (input.type === 'text' || input.tagName.toLowerCase() === 'textarea') clonedInputs[index].value = input.value;
            if (input.type === 'checkbox' || input.type === 'radio') clonedInputs[index].checked = input.checked;
            if (input.tagName.toLowerCase() === 'select'){
                clonedInputs[index].value = input.value;
            }
        });                        
        
        updateGrid(gridParent,elementToAdd);
    }

    placeElementInGrid(elementToAdd, x, y);

    // Ajouter les icônes de poubelle, de déplacement, de configuration et de copie
    addIcons(elementToAdd);

    addIconPickerListener(elementToAdd.querySelector(".stripMask"));

    // Appliquer la classe 'dropped' pour ajuster la taille de l'image
    elementToAdd.classList.add('dropped');

    // rcupere la couleur du widget
    //col = getComputedStyle(elementToAdd.querySelector(".widgetBottomLabel")).backgroundColor;    
    //elementToAdd.style.borderColor = col;    
    // Génère un angle aléatoire entre 30° et 120°
    const randomAngle = getRandomAngle(150, 355);
    // Applique le dégradé avec cet angle
    //elementToAdd.style.background = `linear-gradient(${randomAngle}deg, #545454, #545454, #8a8a8a, #696969,#545454)`;
    elementToAdd.style.background = `linear-gradient(${randomAngle}deg, #101010, #101010, #464646, #252525,#101010)`;
    

    elementToAdd.setAttribute("x",x);
    elementToAdd.setAttribute("y",y);
    
    //_l("dropped: "+sizeX+"x"+sizeY+" ("+x+", "+y+")");    
    parseGrid(gridParent,clearTargetCell);    
    parseGrid(gridParent,identifyGridItem);   // now that the grid layout has changed, we identify again the cells
    
    sortGrid(gridParent.closest(".bank"));
    
    if (dropOrigin == "duplicate") setStrip(elementToAdd.querySelector(".stripMask"));         
    else if (dropOrigin == "move") {
        win = parseInt(elementToAdd.closest(".gridParent").getAttribute("window")) ;          
        bank=elementToAdd.closest(".bank").id;
        _l("transmit move from "+posBeforeMove.x+","+posBeforeMove.y+" to "+x+","+y );
        
        //transmit('moveStrip',{bank:bank,x1:posBeforeMove.x,y1:posBeforeMove.y,x2:x,y2:y,win:win,positions:jsonStripPositions}); 
        postQuery({command:"moveStrip",param:{bank:bank,id:idBeforeMove,x2:x,y2:y,win:win,positions:jsonStripPositions}})
    }
    
    dropOrigin = "move";
}

function getRandomAngle(min, max) {
  return Math.floor(Math.random() * (max - min + 1)) + min;
}

function showStrip(strip){
    strip.style.display = "block"
    layout = strip.closest('.widget').querySelector('.widgetMask');
    layout.style.display = "none";       
}

function hideStrip(strip){
    strip.style.display="none";    
}

function updateGrid(gridParent,elementToAdd){
    gridParent.appendChild(elementToAdd);
    
    const { sizeX, sizeY } = getElementSize(elementToAdd);
    qty = sizeX * sizeY;
    count = 0;
    // remove non widget cells correspnding of the number of cells that the widget has taken
    const children = Array.from(gridParent.children);
    children.forEach(cell => {
        if (!cell.classList.contains('widget')) {
            if (count++ < qty) gridParent.removeChild(cell);
        }
    });
}


function isModified(strip){
    strip.setAttribute('modified','yes')
}

// Fonction pour ajuster la position et la taille d'un widget dans la grille
function placeElementInGrid(element, x, y) {
    const { sizeX, sizeY } = getElementSize(element)
    element.style.gridColumnStart = x + 1;
    element.style.gridRowStart = y + 1;
    element.style.gridColumnEnd = x + sizeX + 1;
    element.style.gridRowEnd = y + sizeY + 1;

    // Ajuste la taille du widget pour qu'il remplisse les cellules
    /*element.style.width = '100%';
    element.style.height = '100%';*/
    element.style.position = 'relative';
}

// Fonction pour calculer les coordonnées de la cellule cliquée
function calculateXY(clientX, clientY, gridParent) {
    //_l("x: "+clientX+" y:"+clientY);
    const rect = gridParent.getBoundingClientRect();
    const {cellSizeX, cellSizeY} = getCellSize(rect);
    x = Math.floor((clientX - rect.left) / cellSizeX);
    y = Math.floor((clientY - rect.top) / cellSizeY);
    if (x<0) x = 0;
    return { x, y };
}

function getCellSize(rect){
    const sX = rect.width / 4;
    const sY = rect.height / 4;
    return {cellSizeX: sX, cellSizeY: sY}
}

// Fonction pour calculer les coordonnées depuis une cellule
function calculateClient(cellX, cellY, gridParent) {
    const rect = gridParent.getBoundingClientRect();
    const {cellSizeX, cellSizeY} = getCellSize(rect);
    x = Math.floor( rect.left +  (cellSizeX*cellX)+cellSizeX/2 ); // return the middle coordinate of the cell
    y = Math.floor( rect.top +  (cellSizeY*cellY)+cellSizeY/2 );
    //_l("cellX: "+cellX+" cellY:"+cellY+" => ("+x+","+y+") ");
    return { clientX:x, clientY:y};
}

// Fonction pour déterminer la taille du widget
function getElementSize(element) {
    if (element.classList.contains('widget-1x4')) return { sizeX: 1, sizeY: 4 };
    if (element.classList.contains('widget-4x1')) return { sizeX: 4, sizeY: 1 };
    if (element.classList.contains('widget-4x4')) return { sizeX: 4, sizeY: 4 };
    return { sizeX: 2, sizeY: 2 };
}

// Fonction pour vérifier si le widget dépasse de la grille
function isWithinGridBounds(x, y, sizeX, sizeY) {
    const maxX = 4;
    const maxY = 4;
    return (x + sizeX <= maxX) && (y + sizeY <= maxY);
}

// Fonction pour vérifier si une collision existe avec un autre widget
function checkCollision(x, y, sizeX, sizeY, gridParent, currentElement = null) {
    
    /*if (currentElement == null) _l("checkCollision("+x+", "+y+", "+sizeX+", "+sizeY+", "+gridParent.classList+")");
    else _l("checkCollision("+x+", "+y+", "+sizeX+", "+sizeY+", "+gridParent.classList+", "+currentElement.classList+")");*/
    
    collide = false;
    
    const allGridItems = gridParent.children;
    for (let item of allGridItems) {
        // Ignorer la collision avec le widget lui-même
        if (currentElement != null) if (item === currentElement) continue;

        const itemX = parseInt(item.style.gridColumnStart) - 1;
        const itemY = parseInt(item.style.gridRowStart) - 1;
        const itemSizeX = parseInt(item.style.gridColumnEnd) - item.style.gridColumnStart;
        const itemSizeY = parseInt(item.style.gridRowEnd) - item.style.gridRowStart;

        //_l("item:<"+(itemX)+","+itemY+"> itemSize:("+itemSizeX+","+itemSizeY+")");

        if (
            itemX < x + sizeX &&
            itemX + itemSizeX > x &&
            itemY < y + sizeY &&
            itemY + itemSizeY > y
        ) {
            collide = true; // Collision détectée
            break;
        }
    }
    //_l("checkCollision("+x+","+y+") return "+collide);
    return collide;
}


function clearTargetCell(elem){
    if (elem == null) return;
    //_l("clear>>"+elem.classList)
    elem.classList.remove('destOK');
    elem.classList.remove('destNOK');
    //previousTarget = null;    
}

function parseGrid(parentGrid, callback){
    gridItems = parentGrid.querySelectorAll('.grid-item');
    gridItems.forEach((gridItem) => {
        callback(gridItem); 
    });            
}

function identifyGridItem(gridItem){
    rec = gridItem.getBoundingClientRect();
    
    gridItem.classList="";
    gridItem.classList.add("grid-item");
    
    options = scanGridItem(rec.left+rec.width/2,rec.top+rec.height/2,gridItem.parentElement);
    
    if (options == 1){ //gridItem.classList.add("freecellauto");
        if (newWidgetMenu["1x4"]) gridItem.classList.add("widget-1x4");
        if (newWidgetMenu["4x1"]) gridItem.classList.add("widget-4x1");
        if (newWidgetMenu["2x2"]) gridItem.classList.add("widget-2x2");
        if (newWidgetMenu["4x4"]) gridItem.classList.add("widget-4x4");  
        gridItem.classList.add("single-widget");
    }  
    else if (options>1) gridItem.classList.add("widget-menu"); 
    else gridItem.classList.add("widget-forbidden"); 
}

function sortGrid(bank){

    uniqueID = 0;

    bank.querySelectorAll(".gridParent").forEach(function (gridParent) {
        
        var paraArr = [].slice.call(gridParent.querySelectorAll(".widget")).sort(function (a, b) {
        //para.sort(function (a, b) {    
            xA = parseInt(a.getAttribute("x"));
            xB = parseInt(b.getAttribute("x"));
            yA = parseInt(a.getAttribute("y"));
            yB = parseInt(b.getAttribute("y"));
    
            //_l("("+xA+","+yA+") vs ("+xB+","+yB+")");
    
        /*if (yA != yB) {
            return yA - yB; // Trie par y en premier (haut -> bas)
        } else {
            return xA - xB; // Trie par x ensuite (gauche -> droite)
        }*/
    
        if (xA != xB) {
            return xA - xB; // gauche droite 
        } else {
            return yA - yB; // puis haut bas
        }
    
        });
        
        //_l("paraArr>>>"+paraArr)
        
        var win = parseInt(gridParent.getAttribute("window")) ;
        
        paraArr.forEach(function (widget) {
            gridParent.appendChild(widget);
            
            uniqueID++;
            if (uniqueID < 10) uniqueID="0"+uniqueID;
            
            widget.querySelector(".uniqueID").textContent = uniqueID; 
            size = widget.getAttribute("size");
            if (size=="4x1") widget.querySelector(".stripID").classList.add("widgetBottomLeft");
            
        });	 
    });

    var i=0;
    jsonStripPositions = [];    
    bank.querySelectorAll(".widget").forEach(function (widget){

        x = parseInt(widget.getAttribute("x"));
        y = parseInt(widget.getAttribute("y"));
        
        uniqueID = parseInt(widget.querySelector(".uniqueID").textContent);
        win = parseInt(widget.closest(".gridParent").getAttribute("window"));

        position = {x:x,y:y,win:win,id:uniqueID}
        jsonStripPositions[i++] = position;

        
    });
    
    
    drawCurve();
    feedShortcuts();
}

function removeWidgetFromGrid(widget){
    addCells(widget);   // recreate empty cells
    grid = widget.closest(".gridParent");
    widget.remove();
    parseGrid(grid,identifyGridItem);   // now that the grid layout has changed, we identify again the cells
    sortGrid(grid.closest(".bank"));    
}

// Fonction pour ajouter les icônes de poubelle, de déplacement, de configuration et de copie à un widget
function addIcons(widget) {
    let trashIcon = document.createElement('div');
    trashIcon.className = 'striptrash-icon trash-icon';
    trashIcon.addEventListener('click', () => {
        trashStrip(widget);
    });
    widget.appendChild(trashIcon);

    let moveIcon = document.createElement('div');
    tip = document.createElement('div');
    tip.className="tooltiptext";
    tipText = i18n.gettext("click&drag to move");
    tip.textContent=tipText;
    moveIcon.appendChild(tip);
    
    moveIcon.className = 'move-icon tooltip';
    moveIcon.addEventListener('mousedown', (event) => {
        event.stopPropagation(); // Empêcher l'événement de propagation pour permettre le drag¨
        dropOrigin = "move";
    });
    /*moveIcon.addEventListener('click', (event) => {
        event.stopPropagation(); // Empêcher l'événement de propagation
        alert(tipText)
        dropOrigin = "move";
    });            */
    widget.appendChild(moveIcon);

    let copyIcon = document.createElement('div');
    tip2 = tip.cloneNode(true);
    tip2text = i18n.gettext("click&drag to copy");
    tip2.textContent = tip2text;
    copyIcon.appendChild(tip2);
    
    copyIcon.className = 'copy-icon tooltip';
    copyIcon.addEventListener('mousedown', (event) => {
        event.stopPropagation(); // Empêcher l'événement de propagation
        dropOrigin = "duplicate"; // Activer le mode de copie
    });
    /*copyIcon.addEventListener('click', (event) => {
        event.stopPropagation(); // Empêcher l'événement de propagation
        alert(tip2Text)
        dropOrigin = "move";
    });*/
    widget.appendChild(copyIcon);

    let configIcon = document.createElement('div');
    configIcon.className = 'config-icon';
    configIcon.addEventListener('click', () => {
        showStrip(widget.querySelector('.stripMask'));
    });
    widget.appendChild(configIcon);
}

function addCells(widget){
    //determine the number of empty cells to recreate
    const { sizeX, sizeY } = getElementSize(widget);
    qty = sizeX * sizeY;
    for (cell=0;cell<qty;cell++){
        widget.insertAdjacentHTML('afterend', '<div class="grid-item"></div>');
                          
    } 
}

function bankNameFree(value, obj){
    bankNames = document.querySelectorAll('.bankName');
    free = true;
    for (var i=0;i<bankNames.length;i++){
        if (bankNames[i].value == value && obj != bankNames[i]){
            free = false;
            break;
        }        
    }
    return free;
}

function updateBankName(obj,oldValue){

    if (!bankNameFree(obj.value, obj)){
        alert(i18n.gettext("Module name already in use"));
        obj.value = oldValue
        return;
    }
    _(oldValue).id = obj.value;
    transmit('renameBank',{oldname:oldValue,newname:obj.value});
    obj.defaultValue = obj.value;
    
    feedShortcuts();
}

function zoomGrid(ratio){
    //if (countBanks() == 0) return;
    //select all grid of the active bank
    grids = document.querySelectorAll('.gridParent');
    grids.forEach(grid => {
        grid.style.width=ratio+"%";
    });    
    drawCurve();
}

function countBanks(){
    return  _("flexibleGridZone").getElementsByClassName("bank").length;    
}


function copyBank(sourceBankName){
    destinationBankname = getBankNewName();
    if (destinationBankname=="") return;
    
    bankDef = {"strips":[]};
    
    widgets = _(sourceBankName).querySelectorAll(".widget");
    
    for (var i=0;i<widgets.length;i++){
        strip = widgets[i].querySelector(".stripMask");
        childs = strip.querySelector(".properties").querySelectorAll(".stripProperty")
        addresses = forgeProperties(childs);
        bankDef.strips[i] = forgeStripJson(strip, addresses);
        //alert(JSON.stringify(bankDef.strips[i]));
    }

    bankSelectors = _("bankNavigator").querySelectorAll(".bankSelector");

    for (var i=0;i<bankSelectors.length;i++) {
        bankSelector = bankSelectors[i];
        bankNameEl = bankSelector.querySelector(".bankName")
        if (bankNameEl.value == sourceBankName ){
            // set bank icon 
            bankDef.icon = bankSelector.querySelector(".bankIcon").textContent;
            break;
        } 
    }
    
    newBank(destinationBankname,bankDef);
}


function trashBank(bankName){
    if (confirm("Are you sure to delete this bank?") == true) {
    //if (confirm(i18n.gettext("Are you sure to delete this module?")) == true) {        
    	transmit('trashBank',{name:bankName});

        bankSelectors = document.querySelectorAll('.bankSelector');
        
        bankSelectors.forEach(bankSelector => {
            bankNameEl = bankSelector.querySelector(".bankName")
            if (bankNameEl.value == bankName ){
                bankSelector.remove();
                noMoreBanks(false);
                if (activeBank == bankName) _("connectionCanvas").style.visibility="hidden"; // hide path if we delete the current displayed bank
                return;
            } 
        }); 

        _(bankName).remove(); 
        
        hideOrShowZoom();      
        
        feedShortcuts();        
    } 
}

function hideOrShowZoom(){
    zoomVisible = "block";
    if (countBanks()==0) zoomVisible = "none";
    _("zoomDiv").style.display = zoomVisible; // hiode or show zoom if banks are visible or not        
}

function noMoreBanks(unlock){
    newBankVisible = "flex";
    bankCopyVisible= "block";

    if (unlock){
        newBankVisible = "none";
        bankCopyVisible= "none"
    }
    if (countBanks()==0) zoomVisible = "none";
    document.querySelector(".newBank").style.display = newBankVisible;
    allBanks = _("bankNavigator").querySelectorAll(".bankCopy");
    for (var i=0;i<allBanks.length;i++) allBanks[i].style.display=bankCopyVisible; 
}


function buildBankContainer(bankName,bankDef){
    bankIcon = bankDef.icon;
    _("flexibleGridZone").insertAdjacentHTML('afterbegin', '<div class="bank" id="'+bankName+'"></div>');
    for (w=0;w<3;w++){ // create 3 windows for each bank
        grid = document.createElement('div'); 
        grid.className ="gridParent"; grid.setAttribute("window",w);
        for (cell=0;cell<16;cell++) grid.insertAdjacentHTML('afterbegin', '<div class="grid-item"></div>'); 
        addGridEventListeners(grid); 
        _(bankName).appendChild(grid);        
    }
    
    _("bankNavigator").insertAdjacentHTML('afterbegin', '<div class="bankSelector"><div class="tooltip"><span class="tooltiptext">'+i18n.gettext("clic to select, double clic to rename")+'</span><input maxlength="12" style="width:100%" size="12" type="text" class="bankName" readonly value="'+bankName+'"></div><button class="charPopupButton bankIcon">'+bankIcon+'</button><span class="bankIcon trash-icon" style="font-size:1.5vw"></span><label class="bankCopy" style="font-size:1.5vw">📋</label></div>'); 
    bankSelector = _("bankNavigator").firstElementChild // get the element we just added
    addBankSelectorListeners(bankSelector);
    addIconPickerListener(bankSelector);
    if (countBanks()==5) noMoreBanks(true);
     _(bankName).addEventListener('scroll', function() {
        if (widgetDropMenuIsOpen) hideWidgetDropMenu(); // we close the menu to avoid that widget gets dropped in between grid
        drawCurve();        // redraw the connection line between strips
    });    
    activeBank = bankName;
}

function addBankSelectorListeners(bankSelectorEl){
        
    var bankName = bankSelectorEl.querySelector(".bankName");

    bankName.ondblclick = function(){
      this.removeAttribute('readonly');
    };    
    
    bankName.onkeypress = function(event){
        if (bankName.value.length >= maxBankNameL) {
            event.preventDefault(); // Empêche la saisie si la limite est atteinte
            return false;
        }        
        return /[a-z\d\-_\s]+$/i.test(event.key)
    };
    
    bankName.onchange = function(){
        this.setAttribute('readonly',true);
        this.blur();
        updateBankName(this,this.defaultValue);
    };           

    bankName.onclick = function(){
        selectBank(this.value);
        //this.blur();
    }; 
    
    trashBankIcon = bankSelectorEl.querySelector('.trash-icon');
    trashBankIcon.onclick = function(event){
        bankNameEl = event.target.parentElement.querySelector(".bankName")
        trashBank(bankNameEl.value);
    };            

    copyBankIcon = bankSelectorEl.querySelector('.bankCopy');
    copyBankIcon.onclick = function(event){
        bankNameEl = event.target.parentElement.querySelector(".bankName")
        copyBank(bankNameEl.value);
    };            

    
}


function addGridEventListeners(grid){
        grid.addEventListener('mouseleave', () => {
            if (previousTarget !=null) clearTargetCell(previousTarget);
        });  
        
        grid.addEventListener("click", (event) => {
        if (event.target.classList.contains('grid-item')){
            if (widgetDropMenuIsOpen) hideWidgetDropMenu();
            else{
                possibleOptions = scanGridItem(event.clientX,event.clientY,grid);
        
                newWidgetMenu["event"] = event;    
        
                if (possibleOptions == 1 ){
                    //alert("drop directly the widget");
                    if (newWidgetMenu["1x4"]) shape=".widget-1x4"
                    if (newWidgetMenu["4x1"]) shape=".widget-4x1"
                    if (newWidgetMenu["2x2"]) shape=".widget-2x2"
                    if (newWidgetMenu["4x4"]) shape=".widget-4x4"
                    createWidget(shape,event,grid,"new")
                }
                else if (possibleOptions>1) showWidgetDropMenu(grid);
                else{
                    event.target.classList.add("destNOK");
                    setTimeout(() => { event.target.classList.remove("destNOK");}, 100);                    
                }
                
            }
        }
        })
      
        grid.addEventListener("mouseover", (event) => {
            //_l(x+","+y+"=>"+event.target.classList);
            if (event.target.classList.contains('grid-item')){        
                if (previousTarget != event.target){
                    clearTargetCell(previousTarget);
                    previousTarget = event.target;
                    /*freeCell = scanGridItem(event.clientX,event.clientY,grid);   
                    if (freeCell > 0) event.target.classList.add("destOK");
                    else event.target.classList.add("destNOK");*/
                    if (event.target.classList.contains("single-widget") || event.target.classList.contains("widget-menu")) event.target.classList.add("destOK");
                    else event.target.classList.add("destNOK");
                    
                }        
            }else if (event.target.classList.contains('widget')){
                clearTargetCell(previousTarget);
            }
        })      
      
        grid.addEventListener('dragover', (event) => {
               
            event.preventDefault();
            
            const { x, y } = calculateXY(event.clientX, event.clientY, grid);
            const { sizeX, sizeY } = getElementSize(draggedElement);
        
        
            if (previousTarget != event.target){
                clearTargetCell(previousTarget);
                if (event.target.classList.contains('grid-item')){
                    if (isWithinGridBounds(x, y, sizeX, sizeY) && !checkCollision(x, y, sizeX, sizeY, grid, draggedElement)) event.target.classList.add("destOK");
                    else event.target.classList.add("destNOK");
                    previousTarget = event.target;                                            
                }else previousTarget = event.target;
                //
            }
        
        
        });
        
        // Gestion du drop dans chaque grille
        grid.addEventListener('drop', (event) => {
            event.preventDefault();
        
            const { x, y } = calculateXY(event.clientX, event.clientY, grid);
            const { sizeX, sizeY } = getElementSize(draggedElement);
        
            if (isWithinGridBounds(x, y, sizeX, sizeY) && !checkCollision(x, y, sizeX, sizeY, grid, draggedElement)) {
                dropWidget(draggedElement,grid,x,y,sizeX, sizeY);
            }
        });      
        
        parseGrid(grid,identifyGridItem);     
}

function closeStripConfig(stripObj){
    if (stripObj.getAttribute("modified") === "yes"){
        setStrip(stripObj);
        stripObj.setAttribute("modified","no");
    }
    hideStrip(stripObj);
    stripObj.closest('.widget').querySelector('.widgetMask').style.display = "block";    
    
    //drawCurve(); // workaround (see sortGrid)
}

function renderStrip(stripObj,stripValues = null){

    //if (stripValues !=null) _l(JSON.stringify(stripValues));

    // first cleanUP any existing strip properties so we can replace them by the one that correspond to the selected type
    props = stripObj.querySelector(".properties");
    props.innerHTML = '';
    
    if (stripValues != null){
        // setting the "fixed" properties = color and icon
        propertiesValues = stripValues["properties"];
    	// setting the color
    	colorID = stripValues["color"];
    	if (colorID !=99) strip.getElementsByClassName("color")[0].options.selectedIndex = colorID;	
        // setting strip type/name
        stripSkelName = stripValues["type"];
        stripObj.querySelector(".skelName").value = stripSkelName;
        icon = stripValues["icon"];
        stripObj.querySelector(".charPopupButton").textContent = icon;
    }else{
        stripSkelName = stripObj.querySelector(".skelName").value; // created from scratch we take the default drop down selection 
        propertiesValues = null;
    } 

    // parsing all stripskeletons definition 
    skel = buildSkeleton(stripSkelName,stripSkeletons,props,propertiesValues,"stripProperty")
    
    icons = stripObj.closest('.widget').querySelectorAll('.adaptativeIcon');

	icons.forEach((icon) => {	
	   icon.textContent = skel["icon"];
	});	    
    
    refreshWidget(stripObj);
}

function getSkeleton(skelName,skeletons){
    for (var i=0; i<skeletons.length;i++){
        skeleton = skeletons[i];
        skelID = Object.keys(skeleton)[0];
        if (skelName === skelID) return skeleton;
    }
}


function buildSkeleton(skelName,skeletonsDef,targetEl,propertiesValues,propClass){

    //_l(skelName+" properties:"+propertiesValues+" => "+JSON.stringify(skeletonsDef));

    emptyElement(targetEl);

    skeleton = getSkeleton(skelName,skeletonsDef);
    // now based on the strip type, we forge the properties and all the dropTargetStrip
    skelPropertiesArray = skeleton.properties;
    //_l("skelPropertiesArray"+JSON.stringify(skelPropertiesArray))
    
    for (prop in Object.keys(skelPropertiesArray)) {
        var pCount = 0;
        for (let address in skelPropertiesArray[prop]) {
            type = skelPropertiesArray[prop][address];
            pCount++;
            let pValue = null;
            // if we received some value, we match them with the properties
            if (propertiesValues !=null){
                // ompare all values we got one by one with the addresses 
                for (p=0;p<propertiesValues.length;p++){
                    //example : comparing /ch/config/02/name /ch/%02d/config/name
                    //sscanf('SN/2350001/hello', 'SN/%d/%s') return [2350001,"hello"]
                    pValue=sscanf(propertiesValues[p],address); // pValue is an array of matched values
                    //_l("type: "+type+" comparing "+propertiesValues[p]+" "+address+" =>"+pValue);                            
                    if (pValue[0] != null) break; //we found at least one value that match so we exit
                }
            }
            clonedNode =  _(type+"TPL").cloneNode(true);
            populateDataCallBack = "populate"+type;
            
            window[populateDataCallBack](clonedNode,address,pValue);   // populatedropdown, populatefreetext

            //clonedNode.style.display = "block";					
			//clonedNode.classList.add(address);
			clonedNode.classList.add(propClass);
            
            // the class determines the behaviour of the properties, hence which function to call when it changes => handleChange
			// by default, 
			var handleChange;
			switch (propClass){
			    case "stripProperty":
			        handleChange = function(event) {refreshWidget(event.target.closest('.stripMask'), true);}
			    break;
			    case "shortCutProperty":
			        handleChange = function(event) {transmitShortcut()}
			    break;
			}
            clonedNode.addEventListener('change', handleChange);
			
			
			clonedNode.setAttribute("address",address); // SET THE TOP STRIP GENERIC ADDRESS 
			clonedNode.removeAttribute("id");	
			
			targetEl.appendChild(clonedNode);	
			
        }
    }
    
    return skeleton;
}


function populateSkeletonsDropDown(skeletons,skelNameDropDown){
    //groupName = new Array("");
    emptyElement(skelNameDropDown);

    skeletons.forEach(function(skeleton) {
        
        //first key:value = id:group

        groupName = skeleton[Object.keys(skeleton)[0]];    
        skelID = Object.keys(skeleton)[0];
        
        // Trouver l'optgroup en fonction de son label
        optgroup = Array.from(skelNameDropDown.getElementsByTagName('optgroup')).find(
            group => group.label === groupName
        );
    
        
        if (!optgroup){
            //créer le groupe si il n'existe pas
            optgroup = document.createElement("optgroup");
            optgroup.label = groupName;
            skelNameDropDown.appendChild(optgroup);
        }
        // et y rattacher l'option
        newOption = document.createElement('option');
        newOption.textContent = skelID;
        optgroup.appendChild(newOption);
    })  
}

function setStrip(strip){
    //_l("setStrip >>> "+strip.classList);
    childs = strip.querySelector(".properties").querySelectorAll(".stripProperty")
    addresses = forgeProperties(childs);
    sortGrid(strip.closest(".bank"));
    
    //transmit('setStrip',forgeStripHeaderValues(strip,addresses));
    postQuery({command:"setStrip", param:forgeStripHeaderValues(strip,addresses)});
}

function trashStrip(widget){
    strip=widget.querySelector(".stripMask");
    bank=widget.closest(".bank").id;
    var pos = forgeStripPosition(strip);
    removeWidgetFromGrid(widget); // we remove the widget, it will be created by the server as a back response    
    const json = {bank:bank,position:pos,positions:jsonStripPositions};    
    
    
//    transmit('trashStrip',json);    
    postQuery({command:"trashStrip", param:json});

    
    
}

function forgeStripHeaderValues(strip, properties){
    var bankName = strip.closest(".bank").id;
    win = parseInt(strip.closest(".gridParent").getAttribute("window")) ;
	jsonResponse = {bank:bankName,strip:forgeStripJson(strip, properties),positions:jsonStripPositions};    
    //_l(JSON.stringify(jsonResponse));
    return jsonResponse;
}

function forgeStripJson(strip, properties){
    //_l("forgeStripJson("+slotContainer.id+","+JSON.stringify(properties)+")")
  
    widget = strip.closest(".widget");

    type = strip.querySelector(".skelName").value;
    icon = strip.querySelector('.charPopupButton').textContent;
    size = widget.getAttribute("size");
    colorDropDown = strip.querySelector(".color");
    color = parseInt(colorDropDown.options[colorDropDown.selectedIndex].value);     
    
    return {type:type,size:size,color:color,icon:icon,properties:properties,position:forgeStripPosition(strip)};
}


function forgeProperties(childrens){
    let addresses = new Array();

    for (var i = 0; i < childrens.length; i++) {
      child = childrens[i];
      type = child.getAttribute("dataSet");
      getAddressValueCallback = type+"AddressValue"; //which function will return the value ?
      //_l(getAddressValueCallback);
      addresses[i] = window[getAddressValueCallback](child);
    }
    return addresses;
}

function freetextAddressValue(textField){
    addressValue = textField.getAttribute("address"); // "/command//ch/level/%s %s" 

    //assuming textFiled.value = /ch/level/03 56    
    arr = textField.value.split(" ");  // arr = ["03","05"]
    addressValue = sprintf(addressValue,arr); // "/command//ch/level/03 56"
    
    return addressValue; 
}




function colorAddressValue(dropDown){
  return "dummyColor";
}

// the below function are callbacks used to retrieve the value of strip properties
function dropdownAddressValue(dropDown){
    selectIndex=dropDown.selectedIndex;
    //_l(dropDown.options[selectIndex].getAttribute("address"))
    
    if (selectIndex == -1){ // dropdown has no data
        // put the focus on the panel where the dropdown is
/*        panelNum = dropDown.closest(".panel").id.split("-")[0];
        _(panelNum).checked = true;
        alert(i18n.gettext("Data is missing, please fix"));
        dropDown.classList.add('tour-frame');        */
        selectIndex = 0;
        
    }else dropDown.classList.remove('tour-frame');        
    
    return dropDown.options[selectIndex].getAttribute("address");    
    
}

function forgeStripPosition(strip){
    widget = strip.closest(".widget");
    
    win = parseInt(widget.closest(".gridParent").getAttribute("window"));

    x = widget.getAttribute("x");
    y = widget.getAttribute("y");
    
    index = parseInt(widget.querySelector(".uniqueID").textContent);
    
    return {win:win,x:x,y:y,id:index};
}


/*
function updateStripDropDown(dropDown){
	let address = new Array(); 
	address[0]=dropdownAddressValue(dropDown);
    strip = dropDown.closest(".stripMask");
    transmit('updateStrip',forgeStripHeaderValues(strip,address));
}

function updateStripText(textField){
	let address = new Array(); 
	address[0]=freetextAddressValue(textField);
    strip = textField.closest(".stripMask");
    transmit('updateStrip',forgeStripHeaderValues(strip,address));
}*/



function getBankNewName(){
    var n=countBanks()+1;
    do{
        do proposedName="bank"+n; while (!bankNameFree(proposedName,n++))
        
            do {
              newName = window.prompt(i18n.gettext("New bank name ")+"(max 12 car.)",proposedName);
            } while (newName && newName.length > maxBankNameL);        
        
        
        if (newName == null || newName == "") return "";
        if (!bankNameFree(newName, "")) alert(i18n.gettext("Module name already in use"));
        
    }while (!bankNameFree(newName, ""));   
    return newName;
}

function newEmptyBank(){
    newName = getBankNewName();
    if (newName=="") return;
    bankDef = {"icon":"3","strips":[]};
    newBank(newName,bankDef);
    hideOrShowZoom();
}

function newBank(newName,bankDef){
    buildBankContainer(newName,bankDef);   
    feedBankWidgets(newName,bankDef)
    selectBank(newName);
    //transmit('newBank',bankDef); // too big to be send by socket so we send it via http post
    postQuery({command:"newBank", param:{[newName]:bankDef}});
    zoomGrid(_("zoom").value);    
}


function restoreStrip(jsonValue){
    //_l(JSON.stringify(jsonValue[0]));    
    jsonContent = jsonValue[0];
    
	win = jsonContent.strip.position.win; 
	
    size = jsonContent.strip.size;
    x = jsonContent.strip.position.x;
    y = jsonContent.strip.position.y;
    
    var grid = _(jsonContent.bank).querySelector('.gridParent[window="'+win+'"]');
    coordinates = calculateClient(x, y, grid);
    
    widget = ".widget-"+size;

    createWidget(widget,coordinates,grid,"restore",jsonContent.strip);    
}

function refreshWidget(strip, setModify=false){
    if (setModify) isModified(strip);
    refreshWidgetColor(strip);
    refreshwidgetLabels(strip)
}

function refreshWidgetColor(strip){
	//_l("refreshWidgetColor("+obj.id+")"); // module J3-slot0	
    //color=strip.getElementsByClassName("\/ch\/color")[0];			// we escape the / by putting \ before
    widget = strip.closest(".widget");
	stripColor=widget.querySelector(".widgetTopLabel");
    dropDown = strip.getElementsByClassName("color")[0];

    if (dropDown.value == 99){
        colorID="1"; // temp
    }else{
    	colorID=dropDown.selectedIndex;
    }  
    colorClass=dropDown.options[colorID].className;	    
	stripColor.className="widgetTopLabel "+colorClass;			
	
    /*col = getComputedStyle(stripColor).backgroundColor;    
    const randomAngle = getRandomAngle(150, 355);
    widget.style.background = `linear-gradient(${randomAngle}deg, ${col}, #545454, #8a8a8a, #696969,#545454)`;*/ 	
	
	
}

function refreshwidgetLabels(strip){

    widget = strip.closest(".widget");
    //take the first properties values as the widget label

    //default bottm wdget label is the strip name
    name = strip.querySelector(".skelName").value;
    widget.querySelector(".widgetTopLabel").textContent = name;

    properties = strip.querySelector(".properties").querySelectorAll('[dataSet]'); 
    var pCount =  properties.length;
    // we only consider the first 2 strip properties to feed the widget label
    if (pCount>2) pCount=2;

    undef = i18n.gettext("undefined")+"!";

    for (var i=0; i<pCount;i++){
        property = properties[i];
        type = property.getAttribute("dataSet");
    
        var label;
        var address;
        switch (type){
            case 'dropdown':
                index = property.selectedIndex;
                if (index != -1){
                    option = property.options[index];
                    label = option.textContent;
                }else{
                    label = undef;
                    option = property.options[0];
                }
                address = option.getAttribute("address");
            break;
            case 'freetext':
                label = property.value;  
                address = property.getAttribute("address");
            break;
        }
        
        if (i==0) widgetLabel = widget.querySelector(".widgetBottomLabel");
        else widgetLabel = widget.querySelector(".widgetTopLabel");
        
        widgetLabel.textContent = label;
        if (label == undef) widgetLabel.classList.add("tour-frame");
        else widgetLabel.classList.remove("tour-frame");
        widgetLabel.setAttribute("address",address);    
        
    }
}

function modifyClosest(obj){
    //alert(obj.classList);
    isModified(obj.closest('.stripMask'));
}

window.onresize = function() {
    // do whatever has to be redrawn
    drawCurve(); 
};

function drawCurve() {
    
  if (isTouchScreen()) return;
  if (countBanks() == 0) return;
  
  //alert(activeBank)
  const gridContainer = _(activeBank);
  //const gridContainer = _("flexibleGridZone");
  
  const canvas = _('connectionCanvas');
  const ctx = canvas.getContext('2d');
  ctx.clearRect(0, 0, canvas.width, canvas.height); // Effacer le canvas

  position = gridContainer.getBoundingClientRect();
  
  // Ajuster la taille et position du canvas pour etre la meme que le conteneur grid
  canvas.style.left = position.left+'px'
  canvas.style.top = gridContainer.style.top+'px' //position.top+'px'; does not work
  
  let yOffset = (position.top - gridContainer.style.top); // for some reason, there s a difference between position reported, hence we have to define a delta 

  canvas.width = position.width;
  canvas.height = position.height;
  
  /*canvas.style.border="5px solid green";
  gridContainer.style.border="5px dashed orange";*/
  
  // design des connexions

  ctx.lineWidth = 8; // Épaisseur de la ligne
  ctx.strokeStyle = 'yellow'; // Couleur de la ligne
  ctx.setLineDash([1, 2]);
  ctx.shadowColor = '#FF5733';
  ctx.globalAlpha = 0.9;
  ctx.shadowBlur = 8;

  const divs = Array.from(_(activeBank).querySelectorAll('.uniqueID')).sort((a, b) => {
    // we collect the id  and sort them
    a = parseInt(a.textContent);
    b = parseInt(b.textContent);
    
    //console.log("a:"+a+", b:"+b);
    
    return a - b;
  });


  for (let i = 0; i < divs.length - 1; i++) {
      
    origin =  divs[i].closest(".widget")//.querySelector(".widgetTopLabel");
    destination = divs[i+1].closest(".widget")//.querySelector(".widgetTopLabel");
      
    const start = origin.getBoundingClientRect();
    const end = destination.getBoundingClientRect();

    // we adjust the offset to position link in the middle of the widget
    startYOffset = yOffset - start.height/2.5;
    endYOffset = yOffset - end.height/2.5;
    startXOffset = start.width/5;
    endXOffset = end.width/5;

    const startX = start.left+startXOffset
    const startY = start.top-startYOffset 
    const endX = end.left+endXOffset  
    const endY = end.top-endYOffset 

    //console.log("start: ("+parseInt(startX)+","+parseInt(startY)+"), end:("+parseInt(endX)+","+parseInt(endY)+")");

    if (endY<0 ||startY<0) return;

    ctx.beginPath();
    ctx.moveTo(startX, startY);

    // Courbe de Bézier pour relier les points
    const controlX = (startX + endX) / 2;
    const controlY = Math.min(startY, endY) - 50; // Ajuster pour la courbure


    ctx.quadraticCurveTo(controlX, controlY, endX, endY);
    ctx.stroke();
    drawArrowhead(ctx, endX, endY, Math.atan2(endY - controlY, endX - controlX));
    drawArrowhead(ctx, startX, startY, Math.atan2(startY - controlY, startX - controlX));
  }
    //sleep(5000).then(() => { _('connectionCanvas').classList.add("fadeOut") });
}

function drawArrowhead(ctx, x, y, angle) {
  const arrowLength = 20; // Longueur de la pointe de flèche
  const arrowWidth = 15;   // Largeur de la pointe de flèche
  ctx.beginPath();
  ctx.moveTo(x, y);
  ctx.lineTo(x - arrowLength * Math.cos(angle - Math.PI / 6), y - arrowLength * Math.sin(angle - Math.PI / 6));
  ctx.lineTo(x - arrowLength * Math.cos(angle + Math.PI / 6), y - arrowLength * Math.sin(angle + Math.PI / 6));
  ctx.closePath();
  ctx.fillStyle = ctx.strokeStyle;
  ctx.fill();
}

function toggleCanvas(el){
    _('connectionCanvas').classList.toggle("fadeOut");
    el.classList.toggle("bankSelected");
    el.classList.toggle("lightOn");
    el.classList.toggle("lightOff");
    //shutdown the path after few seconds
    //if (_("path").classList.contains("lightOn")) sleep("4000").then(() => { toggleCanvas(_("path")) });

    // animate the path
    /*if (_("path").classList.contains("lightOn")){
        canvasAnimation = setInterval(function() {
            drawCurve();
        }, 500)
    }else clearInterval(canvasAnimation);*/
    
}

function getStripID(){
    stripIDs = [];
    
    // determine
    
    allWidgets = _("flexibleGridZone").querySelectorAll('.widget');
    
    for (var i=0;i<allWidgets.length;i++){
        
        widget = allWidgets[i];
        // to issue the strip ID we concatenate bankName and the 2 labels
        stripIDs[i] = widget.closest(".bank").id + ",strip";
        stripIDs[i] += widget.querySelector(".uniqueID").textContent;
        /*stripIDs[i] += widget.querySelector(".widgetTopLabel").textContent + "/";
        stripIDs[i] += widget.querySelector(".widgetBottomLabel").textContent;*/

    } 
    return stripIDs;
}

function getModuleNames(){
    bankName = [];
    allBanks = document.querySelectorAll('.bank');
    for (var i=0;i<allBanks.length;i++) bankName[i]=allBanks[i].id;
    return bankName;
}
