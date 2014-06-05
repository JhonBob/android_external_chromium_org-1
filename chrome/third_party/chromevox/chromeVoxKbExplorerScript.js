var goog=goog||{};goog.global=this;goog.isDef=function(a){return void 0!==a};goog.exportPath_=function(a,b,c){a=a.split(".");c=c||goog.global;a[0]in c||!c.execScript||c.execScript("var "+a[0]);for(var d;a.length&&(d=a.shift());)!a.length&&goog.isDef(b)?c[d]=b:c=c[d]?c[d]:c[d]={}};goog.define=function(a,b){var c=b;goog.exportPath_(a,c)};goog.DEBUG=!1;goog.LOCALE="en";goog.TRUSTED_SITE=!0;goog.STRICT_MODE_COMPATIBLE=!1;goog.provide=function(a){goog.exportPath_(a)};
goog.setTestOnly=function(a){if(!goog.DEBUG)throw a=a||"",Error("Importing test-only code into non-debug environment"+(a?": "+a:"."));};goog.forwardDeclare=function(){};goog.getObjectByName=function(a,b){for(var c=a.split("."),d=b||goog.global,e;e=c.shift();)if(goog.isDefAndNotNull(d[e]))d=d[e];else return null;return d};goog.globalize=function(a,b){var c=b||goog.global,d;for(d in a)c[d]=a[d]};
goog.addDependency=function(a,b,c){if(goog.DEPENDENCIES_ENABLED){var d;a=a.replace(/\\/g,"/");for(var e=goog.dependencies_,h=0;d=b[h];h++)e.nameToPath[d]=a,a in e.pathToNames||(e.pathToNames[a]={}),e.pathToNames[a][d]=!0;for(d=0;b=c[d];d++)a in e.requires||(e.requires[a]={}),e.requires[a][b]=!0}};goog.useStrictRequires=!1;goog.ENABLE_DEBUG_LOADER=!0;goog.require=function(){};goog.basePath="";goog.nullFunction=function(){};goog.identityFunction=function(a){return a};
goog.abstractMethod=function(){throw Error("unimplemented abstract method");};goog.addSingletonGetter=function(a){a.getInstance=function(){if(a.instance_)return a.instance_;goog.DEBUG&&(goog.instantiatedSingletons_[goog.instantiatedSingletons_.length]=a);return a.instance_=new a}};goog.instantiatedSingletons_=[];goog.DEPENDENCIES_ENABLED=!1;
goog.DEPENDENCIES_ENABLED&&(goog.included_={},goog.dependencies_={pathToNames:{},nameToPath:{},requires:{},visited:{},written:{}},goog.inHtmlDocument_=function(){var a=goog.global.document;return"undefined"!=typeof a&&"write"in a},goog.findBasePath_=function(){if(goog.global.CLOSURE_BASE_PATH)goog.basePath=goog.global.CLOSURE_BASE_PATH;else if(goog.inHtmlDocument_())for(var a=goog.global.document,a=a.getElementsByTagName("script"),b=a.length-1;0<=b;--b){var c=a[b].src,d=c.lastIndexOf("?"),d=-1==d?
c.length:d;if("base.js"==c.substr(d-7,7)){goog.basePath=c.substr(0,d-7);break}}},goog.importScript_=function(a){var b=goog.global.CLOSURE_IMPORT_SCRIPT||goog.writeScriptTag_;!goog.dependencies_.written[a]&&b(a)&&(goog.dependencies_.written[a]=!0)},goog.writeScriptTag_=function(a){if(goog.inHtmlDocument_()){var b=goog.global.document;if("complete"==b.readyState){if(b=/\bdeps.js$/.test(a))return!1;throw Error('Cannot write "'+a+'" after document load');}b.write('<script type="text/javascript" src="'+
a+'">\x3c/script>');return!0}return!1},goog.writeScripts_=function(){function a(e){if(!(e in d.written)){if(!(e in d.visited)&&(d.visited[e]=!0,e in d.requires))for(var f in d.requires[e])if(!goog.isProvided_(f))if(f in d.nameToPath)a(d.nameToPath[f]);else throw Error("Undefined nameToPath for "+f);e in c||(c[e]=!0,b.push(e))}}var b=[],c={},d=goog.dependencies_,e;for(e in goog.included_)d.written[e]||a(e);for(e=0;e<b.length;e++)if(b[e])goog.importScript_(goog.basePath+b[e]);else throw Error("Undefined script input");
},goog.getPathFromDeps_=function(a){return a in goog.dependencies_.nameToPath?goog.dependencies_.nameToPath[a]:null},goog.findBasePath_(),goog.global.CLOSURE_NO_DEPS||goog.importScript_(goog.basePath+"deps.js"));
goog.typeOf=function(a){var b=typeof a;if("object"==b)if(a){if(a instanceof Array)return"array";if(a instanceof Object)return b;var c=Object.prototype.toString.call(a);if("[object Window]"==c)return"object";if("[object Array]"==c||"number"==typeof a.length&&"undefined"!=typeof a.splice&&"undefined"!=typeof a.propertyIsEnumerable&&!a.propertyIsEnumerable("splice"))return"array";if("[object Function]"==c||"undefined"!=typeof a.call&&"undefined"!=typeof a.propertyIsEnumerable&&!a.propertyIsEnumerable("call"))return"function"}else return"null";
else if("function"==b&&"undefined"==typeof a.call)return"object";return b};goog.isNull=function(a){return null===a};goog.isDefAndNotNull=function(a){return null!=a};goog.isArray=function(a){return"array"==goog.typeOf(a)};goog.isArrayLike=function(a){var b=goog.typeOf(a);return"array"==b||"object"==b&&"number"==typeof a.length};goog.isDateLike=function(a){return goog.isObject(a)&&"function"==typeof a.getFullYear};goog.isString=function(a){return"string"==typeof a};
goog.isBoolean=function(a){return"boolean"==typeof a};goog.isNumber=function(a){return"number"==typeof a};goog.isFunction=function(a){return"function"==goog.typeOf(a)};goog.isObject=function(a){var b=typeof a;return"object"==b&&null!=a||"function"==b};goog.getUid=function(a){return a[goog.UID_PROPERTY_]||(a[goog.UID_PROPERTY_]=++goog.uidCounter_)};goog.hasUid=function(a){return!!a[goog.UID_PROPERTY_]};goog.removeUid=function(a){"removeAttribute"in a&&a.removeAttribute(goog.UID_PROPERTY_);try{delete a[goog.UID_PROPERTY_]}catch(b){}};
goog.UID_PROPERTY_="closure_uid_"+(1E9*Math.random()>>>0);goog.uidCounter_=0;goog.getHashCode=goog.getUid;goog.removeHashCode=goog.removeUid;goog.cloneObject=function(a){var b=goog.typeOf(a);if("object"==b||"array"==b){if(a.clone)return a.clone();var b="array"==b?[]:{},c;for(c in a)b[c]=goog.cloneObject(a[c]);return b}return a};goog.bindNative_=function(a,b,c){return a.call.apply(a.bind,arguments)};
goog.bindJs_=function(a,b,c){if(!a)throw Error();if(2<arguments.length){var d=Array.prototype.slice.call(arguments,2);return function(){var c=Array.prototype.slice.call(arguments);Array.prototype.unshift.apply(c,d);return a.apply(b,c)}}return function(){return a.apply(b,arguments)}};goog.bind=function(a,b,c){goog.bind=Function.prototype.bind&&-1!=Function.prototype.bind.toString().indexOf("native code")?goog.bindNative_:goog.bindJs_;return goog.bind.apply(null,arguments)};
goog.partial=function(a,b){var c=Array.prototype.slice.call(arguments,1);return function(){var b=c.slice();b.push.apply(b,arguments);return a.apply(this,b)}};goog.mixin=function(a,b){for(var c in b)a[c]=b[c]};goog.now=goog.TRUSTED_SITE&&Date.now||function(){return+new Date};
goog.globalEval=function(a){if(goog.global.execScript)goog.global.execScript(a,"JavaScript");else if(goog.global.eval)if(null==goog.evalWorksForGlobals_&&(goog.global.eval("var _et_ = 1;"),"undefined"!=typeof goog.global._et_?(delete goog.global._et_,goog.evalWorksForGlobals_=!0):goog.evalWorksForGlobals_=!1),goog.evalWorksForGlobals_)goog.global.eval(a);else{var b=goog.global.document,c=b.createElement("script");c.type="text/javascript";c.defer=!1;c.appendChild(b.createTextNode(a));b.body.appendChild(c);
b.body.removeChild(c)}else throw Error("goog.globalEval not available");};goog.evalWorksForGlobals_=null;goog.getCssName=function(a,b){var c=function(a){return goog.cssNameMapping_[a]||a},d=function(a){a=a.split("-");for(var b=[],d=0;d<a.length;d++)b.push(c(a[d]));return b.join("-")},d=goog.cssNameMapping_?"BY_WHOLE"==goog.cssNameMappingStyle_?c:d:function(a){return a};return b?a+"-"+d(b):d(a)};goog.setCssNameMapping=function(a,b){goog.cssNameMapping_=a;goog.cssNameMappingStyle_=b};
goog.getMsg=function(a,b){var c=b||{},d;for(d in c){var e=(""+c[d]).replace(/\$/g,"$$$$");a=a.replace(new RegExp("\\{\\$"+d+"\\}","gi"),e)}return a};goog.getMsgWithFallback=function(a){return a};goog.exportSymbol=function(a,b,c){goog.exportPath_(a,b,c)};goog.exportProperty=function(a,b,c){a[b]=c};
goog.inherits=function(a,b){function c(){}c.prototype=b.prototype;a.superClass_=b.prototype;a.prototype=new c;a.prototype.constructor=a;a.base=function(a,c,h){var f=Array.prototype.slice.call(arguments,2);return b.prototype[c].apply(a,f)}};
goog.base=function(a,b,c){var d=arguments.callee.caller;if(goog.STRICT_MODE_COMPATIBLE||goog.DEBUG&&!d)throw Error("arguments.caller not defined.  goog.base() cannot be used with strict mode code. See http://www.ecma-international.org/ecma-262/5.1/#sec-C");if(d.superClass_)return d.superClass_.constructor.apply(a,Array.prototype.slice.call(arguments,1));for(var e=Array.prototype.slice.call(arguments,2),h=!1,f=a.constructor;f;f=f.superClass_&&f.superClass_.constructor)if(f.prototype[b]===d)h=!0;else if(h)return f.prototype[b].apply(a,
e);if(a[b]===d)return a.constructor.prototype[b].apply(a,e);throw Error("goog.base called from a method of one name to a method of a different name");};goog.scope=function(a){a.call(goog.global)};goog.MODIFY_FUNCTION_PROTOTYPES=!0;
goog.MODIFY_FUNCTION_PROTOTYPES&&(Function.prototype.bind=Function.prototype.bind||function(a,b){if(1<arguments.length){var c=Array.prototype.slice.call(arguments,1);c.unshift(this,a);return goog.bind.apply(null,c)}return goog.bind(this,a)},Function.prototype.partial=function(a){var b=Array.prototype.slice.call(arguments);b.unshift(this,null);return goog.bind.apply(null,b)},Function.prototype.inherits=function(a){goog.inherits(this,a)},Function.prototype.mixin=function(a){goog.mixin(this.prototype,
a)});goog.defineClass=function(a,b){var c=b.constructor,d=b.statics;if(!c||c==Object.prototype.constructor)throw Error("constructor property is required.");c=goog.defineClass.createSealingConstructor_(c);a&&goog.inherits(c,a);delete b.constructor;delete b.statics;goog.defineClass.applyProperties_(c.prototype,b);null!=d&&(d instanceof Function?d(c):goog.defineClass.applyProperties_(c,d));return c};goog.defineClass.SEAL_CLASS_INSTANCES=goog.DEBUG;
goog.defineClass.createSealingConstructor_=function(a){if(goog.defineClass.SEAL_CLASS_INSTANCES&&Object.seal instanceof Function){var b=function(){var c=a.apply(this,arguments)||this;this.constructor===b&&Object.seal(c);return c};return b}return a};goog.defineClass.OBJECT_PROTOTYPE_FIELDS_="constructor hasOwnProperty isPrototypeOf propertyIsEnumerable toLocaleString toString valueOf".split(" ");
goog.defineClass.applyProperties_=function(a,b){for(var c in b)Object.prototype.hasOwnProperty.call(b,c)&&(a[c]=b[c]);for(var d=0;d<goog.defineClass.OBJECT_PROTOTYPE_FIELDS_.length;d++)c=goog.defineClass.OBJECT_PROTOTYPE_FIELDS_[d],Object.prototype.hasOwnProperty.call(b,c)&&(a[c]=b[c])};var cvox={VERBOSITY_VERBOSE:0,VERBOSITY_BRIEF:1,ChromeVox:function(){}};cvox.ChromeVox.host=null;cvox.ChromeVox.msgs=null;cvox.ChromeVox.isActive=!0;cvox.ChromeVox.version=null;cvox.ChromeVox.earcons=null;cvox.ChromeVox.navigationManager=null;cvox.ChromeVox.serializer=null;cvox.ChromeVox.isStickyPrefOn=!1;cvox.ChromeVox.stickyOverride=null;cvox.ChromeVox.keyPrefixOn=!1;cvox.ChromeVox.verbosity=cvox.VERBOSITY_VERBOSE;cvox.ChromeVox.typingEcho=0;cvox.ChromeVox.keyEcho={};cvox.ChromeVox.position={};
cvox.ChromeVox.isChromeOS=-1!=navigator.userAgent.indexOf("CrOS");cvox.ChromeVox.isMac=-1!=navigator.platform.indexOf("Mac");cvox.ChromeVox.modKeyStr=cvox.ChromeVox.isChromeOS?"Shift+Search":cvox.ChromeVox.isMac?"Ctrl+Cmd":"Shift+Alt";cvox.ChromeVox.sequenceSwitchKeyCodes=[];cvox.ChromeVox.visitedUrls={};cvox.ChromeVox.markInUserCommand=function(){};cvox.ChromeVox.syncToNode=function(){};cvox.ChromeVox.speakNode=function(){};cvox.ChromeVox.executeUserCommand=function(){};
cvox.ChromeVox.entireDocumentIsHidden=!1;cvox.ChromeVox.storeOn=function(a){a.isStickyPrefOn=cvox.ChromeVox.isStickyPrefOn;cvox.ChromeVox.navigationManager.storeOn(a)};cvox.ChromeVox.readFrom=function(a){cvox.ChromeVox.isStickyPrefOn=a.isStickyPrefOn;cvox.ChromeVox.navigationManager.readFrom(a)};cvox.ChromeVox.isStickyModeOn=function(){return null!==cvox.ChromeVox.stickyOverride?cvox.ChromeVox.stickyOverride:cvox.ChromeVox.isStickyPrefOn};cvox.KeySequence=function(a,b,c,d){this.doubleTap=!!d;this.cvoxModifier=void 0==b?this.isCVoxModifierActive(a):b;this.stickyMode=!!a.stickyMode;this.prefixKey=!!a.keyPrefix;this.skipStripping=!!c;if(this.stickyMode&&this.prefixKey)throw"Prefix key and sticky mode cannot both be enabled: "+a;a=this.resolveChromeOSSpecialKeys_(a);this.keys={ctrlKey:[],searchKeyHeld:[],altKey:[],altGraphKey:[],shiftKey:[],metaKey:[],keyCode:[]};this.extractKey_(a)};
cvox.KeySequence.KEY_PRESS_CODE={39:222,44:188,45:189,46:190,47:191,59:186,91:219,92:220,93:221};cvox.KeySequence.doubleTapCache=[];cvox.KeySequence.prototype.addKeyEvent=function(a){if(1<this.keys.keyCode.length)return!1;this.extractKey_(a);return!0};cvox.KeySequence.prototype.equals=function(a){if(!this.checkKeyEquality_(a)||this.doubleTap!=a.doubleTap)return!1;if(this.cvoxModifier===a.cvoxModifier)return!0;a=this.cvoxModifier?a:this;return a.stickyMode||a.prefixKey};
cvox.KeySequence.prototype.extractKey_=function(a){for(var b in this.keys)if("keyCode"==b){var c;"keypress"==a.type&&97<=a[b]&&122>=a[b]?c=a[b]-32:"keypress"==a.type&&(c=cvox.KeySequence.KEY_PRESS_CODE[a[b]]);this.keys[b].push(c||a[b])}else this.isKeyModifierActive(a,b)?this.keys[b].push(!0):this.keys[b].push(!1);this.cvoxModifier&&this.rationalizeKeys_()};
cvox.KeySequence.prototype.rationalizeKeys_=function(){if(!this.skipStripping){var a=cvox.ChromeVox.modKeyStr.split(/\+/g),b=this.keys.keyCode.length-1;-1!=a.indexOf("Ctrl")&&(this.keys.ctrlKey[b]=!1);-1!=a.indexOf("Alt")&&(this.keys.altKey[b]=!1);-1!=a.indexOf("Shift")&&(this.keys.shiftKey[b]=!1);var c=this.getMetaKeyName_();if(-1!=a.indexOf(c))if("Search"==c)this.keys.searchKeyHeld[b]=!1;else if("Cmd"==c||"Win"==c)this.keys.metaKey[b]=!1}};
cvox.KeySequence.prototype.getMetaKeyName_=function(){return cvox.ChromeVox.isChromeOS?"Search":cvox.ChromeVox.isMac?"Cmd":"Win"};cvox.KeySequence.prototype.checkKeyEquality_=function(a){for(var b in this.keys)for(var c=this.keys[b].length;c--;)if(this.keys[b][c]!==a.keys[b][c])return!1;return!0};cvox.KeySequence.prototype.length=function(){return this.keys.keyCode.length};cvox.KeySequence.prototype.isModifierKey=function(a){return 16==a||17==a||18==a||91==a||93==a};
cvox.KeySequence.prototype.isCVoxModifierActive=function(a){var b=cvox.ChromeVox.modKeyStr.split(/\+/g);this.isKeyModifierActive(a,"ctrlKey")&&(b=b.filter(function(a){return"Ctrl"!=a}));this.isKeyModifierActive(a,"altKey")&&(b=b.filter(function(a){return"Alt"!=a}));this.isKeyModifierActive(a,"shiftKey")&&(b=b.filter(function(a){return"Shift"!=a}));if(this.isKeyModifierActive(a,"metaKey")||this.isKeyModifierActive(a,"searchKeyHeld"))var c=this.getMetaKeyName_(),b=b.filter(function(a){return a!=c});
return 0==b.length};cvox.KeySequence.prototype.isKeyModifierActive=function(a,b){switch(b){case "ctrlKey":return a.ctrlKey||17==a.keyCode;case "altKey":return a.altKey||18==a.keyCode;case "shiftKey":return a.shiftKey||16==a.keyCode;case "metaKey":return a.metaKey||!cvox.ChromeVox.isChromeOS&&91==a.keyCode;case "searchKeyHeld":return cvox.ChromeVox.isChromeOS&&91==a.keyCode||a.searchKeyHeld}return!1};
cvox.KeySequence.deserialize=function(a){var b={};b.stickyMode=void 0==a.stickyMode?!1:a.stickyMode;b.prefixKey=void 0==a.prefixKey?!1:a.prefixKey;var c=1<a.keys.keyCode.length,d={},e;for(e in a.keys)b[e]=a.keys[e][0],c&&(d[e]=a.keys[e][1]);e=new cvox.KeySequence(b,a.cvoxModifier,!0,a.doubleTap);c&&(cvox.ChromeVox.sequenceSwitchKeyCodes.push(new cvox.KeySequence(b,a.cvoxModifier)),e.addKeyEvent(d));a.doubleTap&&cvox.KeySequence.doubleTapCache.push(e);return e};
cvox.KeySequence.fromStr=function(a){var b={},c={},d;d=-1==a.indexOf(">")?!1:!0;var e=!1;b.stickyMode=!1;b.prefixKey=!1;a=a.split("+");for(var h=0;h<a.length;h++)for(var f=a[h].split(">"),g=0;g<f.length;g++){if("#"==f[g].charAt(0)){var k=parseInt(f[g].substr(1),10);0<g?c.keyCode=k:b.keyCode=k}k=f[g];1==f[g].length?0<g?c.keyCode=f[g].charCodeAt(0):b.keyCode=f[g].charCodeAt(0):(0<g?cvox.KeySequence.setModifiersOnEvent_(k,c):cvox.KeySequence.setModifiersOnEvent_(k,b),"Cvox"==k&&(e=!0))}b=new cvox.KeySequence(b,
e);d&&b.addKeyEvent(c);return b};cvox.KeySequence.setModifiersOnEvent_=function(a,b){"Ctrl"==a?(b.ctrlKey=!0,b.keyCode=17):"Alt"==a?(b.altKey=!0,b.keyCode=18):"Shift"==a?(b.shiftKey=!0,b.keyCode=16):"Search"==a?(b.searchKeyHeld=!0,b.keyCode=91):"Cmd"==a?(b.metaKey=!0,b.keyCode=91):"Win"==a?(b.metaKey=!0,b.keyCode=91):"Insert"==a&&(b.keyCode=45)};
cvox.KeySequence.prototype.resolveChromeOSSpecialKeys_=function(a){if(!this.cvoxModifier||this.stickyMode||this.prefixKey||!cvox.ChromeVox.isChromeOS)return a;var b={},c;for(c in a)b[c]=a[c];switch(b.keyCode){case 33:b.keyCode=38;break;case 34:b.keyCode=40;break;case 35:b.keyCode=39;break;case 36:b.keyCode=37;break;case 45:b.keyCode=190;break;case 46:b.keyCode=8;break;case 112:b.keyCode=49;break;case 113:b.keyCode=50;break;case 114:b.keyCode=51;break;case 115:b.keyCode=52;break;case 116:b.keyCode=
53;break;case 117:b.keyCode=54;break;case 118:b.keyCode=55;break;case 119:b.keyCode=56;break;case 120:b.keyCode=57;break;case 121:b.keyCode=48;break;case 122:b.keyCode=189;break;case 123:b.keyCode=187}return b};cvox.KeyUtil=function(){};cvox.KeyUtil.modeKeyPressTime=0;cvox.KeyUtil.sequencing=!1;cvox.KeyUtil.prevKeySequence=null;cvox.KeyUtil.stickyKeySequence=null;cvox.KeyUtil.maxSeqLength=2;
cvox.KeyUtil.keyEventToKeySequence=function(a){var b=cvox.KeyUtil;b.prevKeySequence&&b.maxSeqLength==b.prevKeySequence.length()&&(b.sequencing=!1,b.prevKeySequence=null);var c=b.sequencing||a.keyPrefix||a.stickyMode,d=new cvox.KeySequence(a),e=d.cvoxModifier;if(c||e){if(!b.sequencing&&b.isSequenceSwitchKeyCode(d))return b.sequencing=!0,b.prevKeySequence=d;if(b.sequencing){if(b.prevKeySequence.addKeyEvent(a))return d=b.prevKeySequence,b.prevKeySequence=null,b.sequencing=!1,d;throw"Think sequencing is enabled, yet util.prevKeySequence alreadyhas two key codes"+
b.prevKeySequence;}}else b.sequencing=!1;c=(new Date).getTime();if(cvox.KeyUtil.isDoubleTapKey(d)&&b.prevKeySequence&&d.equals(b.prevKeySequence)&&(e=b.modeKeyPressTime,0<e&&300>c-e))return d=b.prevKeySequence,d.doubleTap=!0,b.prevKeySequence=null,b.sequencing=!1,cvox.ChromeVox.isChromeOS&&a.keyCode==cvox.KeyUtil.getStickyKeyCode()&&(cvox.ChromeVox.searchKeyHeld=!1),d;b.prevKeySequence=d;b.modeKeyPressTime=c;return d};
cvox.KeyUtil.keyCodeToString=function(a){return 17==a?"Ctrl":18==a?"Alt":16==a?"Shift":91==a||93==a?cvox.ChromeVox.isChromeOS?"Search":cvox.ChromeVox.isMac?"Cmd":"Win":45==a?"Insert":65<=a&&90>=a?String.fromCharCode(a):48<=a&&57>=a?String.fromCharCode(a):"#"+a};cvox.KeyUtil.modStringToKeyCode=function(a){switch(a){case "Ctrl":return 17;case "Alt":return 18;case "Shift":return 16;case "Cmd":case "Win":return 91}return-1};
cvox.KeyUtil.cvoxModKeyCodes=function(){var a=cvox.ChromeVox.modKeyStr.split(/\+/g);return a=a.map(function(a){return cvox.KeyUtil.modStringToKeyCode(a)})};cvox.KeyUtil.isSequenceSwitchKeyCode=function(a){for(var b=0;b<cvox.ChromeVox.sequenceSwitchKeyCodes.length;b++){var c=cvox.ChromeVox.sequenceSwitchKeyCodes[b];if(c.equals(a))return!0}return!1};
cvox.KeyUtil.getReadableNameForKeyCode=function(a){if(0==a)return"Power button";if(17==a)return"Control";if(18==a)return"Alt";if(16==a)return"Shift";if(9==a)return"Tab";if(91==a||93==a)return cvox.ChromeVox.isChromeOS?"Search":cvox.ChromeVox.isMac?"Cmd":"Win";if(8==a)return"Backspace";if(32==a)return"Space";if(35==a)return"end";if(36==a)return"home";if(37==a)return"Left arrow";if(38==a)return"Up arrow";if(39==a)return"Right arrow";if(40==a)return"Down arrow";if(45==a)return"Insert";if(13==a)return"Enter";
if(27==a)return"Escape";if(112==a)return cvox.ChromeVox.isChromeOS?"Back":"F1";if(113==a)return cvox.ChromeVox.isChromeOS?"Forward":"F2";if(114==a)return cvox.ChromeVox.isChromeOS?"Refresh":"F3";if(115==a)return cvox.ChromeVox.isChromeOS?"Toggle full screen":"F4";if(116==a)return"F5";if(117==a)return"F6";if(118==a)return"F7";if(119==a)return"F8";if(120==a)return"F9";if(121==a)return"F10";if(122==a)return"F11";if(123==a)return"F12";if(186==a)return"Semicolon";if(187==a)return"Equal sign";if(188==a)return"Comma";
if(189==a)return"Dash";if(190==a)return"Period";if(191==a)return"Forward slash";if(192==a)return"Grave accent";if(219==a)return"Open bracket";if(220==a)return"Back slash";if(221==a)return"Close bracket";if(222==a)return"Single quote";if(115==a)return"Toggle full screen";if(48<=a&&90>=a)return String.fromCharCode(a)};cvox.KeyUtil.getStickyKeyCode=function(){var a=45;if(cvox.ChromeVox.isChromeOS||cvox.ChromeVox.isMac)a=91;return a};
cvox.KeyUtil.getStickyKeySequence=function(){if(null==cvox.KeyUtil.stickyKeySequence){var a=cvox.KeyUtil.getStickyKeyCode(),a={keyCode:a,stickyMode:!0},b=new cvox.KeySequence(a);b.addKeyEvent(a);cvox.KeyUtil.stickyKeySequence=b}return cvox.KeyUtil.stickyKeySequence};cvox.KeyUtil.getReadableNameForStr=function(){return null};
cvox.KeyUtil.keySequenceToString=function(a,b,c){for(var d="",e=a.length(),h=0;h<e;h++){""==d||c?""!=d&&(d+="+"):d+=">";var f="",g;for(g in a.keys)if(a.keys[g][h]){var k="";switch(g){case "ctrlKey":k="Ctrl";break;case "searchKeyHeld":k=cvox.KeyUtil.getReadableNameForKeyCode(91);break;case "altKey":k="Alt";break;case "altGraphKey":k="AltGraph";break;case "shiftKey":k="Shift";break;case "metaKey":k=cvox.KeyUtil.getReadableNameForKeyCode(91);break;case "keyCode":var l=a.keys[g][h];a.isModifierKey(l)||
c||(f=b?f+cvox.KeyUtil.getReadableNameForKeyCode(l):f+cvox.KeyUtil.keyCodeToString(l))}-1==d.indexOf(k)&&(f+=k+"+")}d+=f;"+"==d[d.length-1]&&(d=d.slice(0,-1))}a.cvoxModifier||a.prefixKey?d=""!=d?"Cvox+"+d:"Cvox":a.stickyMode&&(">"==d[d.length-1]&&(d=d.slice(0,-1)),d=d+"+"+d);return d};cvox.KeyUtil.isDoubleTapKey=function(a){var b=!1,c=a.doubleTap;a.doubleTap=!0;for(var d=0,e;e=cvox.KeySequence.doubleTapCache[d];d++)if(e.equals(a)){b=!0;break}a.doubleTap=c;return b};cvox.KbExplorer=function(){};cvox.KbExplorer.init=function(){document.addEventListener("keydown",cvox.KbExplorer.onKeyDown,!1);document.addEventListener("keyup",cvox.KbExplorer.onKeyUp,!1);document.addEventListener("keypress",cvox.KbExplorer.onKeyPress,!1)};cvox.KbExplorer.onKeyDown=function(a){chrome.extension.getBackgroundPage().speak(cvox.KeyUtil.getReadableNameForKeyCode(a.keyCode),!1,{});a.preventDefault();a.stopPropagation()};cvox.KbExplorer.onKeyUp=function(a){a.preventDefault();a.stopPropagation()};
cvox.KbExplorer.onKeyPress=function(a){a.preventDefault();a.stopPropagation()};document.addEventListener("DOMContentLoaded",function(){cvox.KbExplorer.init()},!1);
