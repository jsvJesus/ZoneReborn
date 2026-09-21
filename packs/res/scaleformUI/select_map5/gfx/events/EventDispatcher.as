class gfx.events.EventDispatcher
{
   var _listeners;
   static var _instance;
   function EventDispatcher()
   {
   }
   static function initialize(target)
   {
      if(gfx.events.EventDispatcher._instance == undefined)
      {
         gfx.events.EventDispatcher._instance = new gfx.events.EventDispatcher();
      }
      target.dispatchEvent = gfx.events.EventDispatcher._instance.dispatchEvent;
      target.dispatchQueue = gfx.events.EventDispatcher._instance.dispatchQueue;
      target.hasEventListener = gfx.events.EventDispatcher._instance.hasEventListener;
      target.addEventListener = gfx.events.EventDispatcher._instance.addEventListener;
      target.removeEventListener = gfx.events.EventDispatcher._instance.removeEventListener;
      target.removeAllEventListeners = gfx.events.EventDispatcher._instance.removeAllEventListeners;
      target.cleanUpEvents = gfx.events.EventDispatcher._instance.cleanUpEvents;
      _global.ASSetPropFlags(target,"dispatchQueue",1);
   }
   static function indexOfListener(listeners, scope, callBack)
   {
      var l = listeners.length;
      var i = -1;
      while(++i < l)
      {
         var listener = listeners[i];
         if(listener.listenerObject == scope && listener.listenerFunction == callBack)
         {
            return i;
         }
      }
      return -1;
   }
   function addEventListener(event, scope, callBack)
   {
      if(this._listeners == undefined)
      {
         this._listeners = {};
         _global.ASSetPropFlags(this,"_listeners",1);
      }
      var listeners = this._listeners[event];
      if(listeners == undefined)
      {
         this._listeners[event] = listeners = [];
      }
      if(gfx.events.EventDispatcher.indexOfListener(listeners,scope,callBack) == -1)
      {
         listeners.push({listenerObject:scope,listenerFunction:callBack});
      }
   }
   function removeEventListener(event, scope, callBack)
   {
      var listeners = this._listeners[event];
      if(listeners == undefined)
      {
         return undefined;
      }
      var index = gfx.events.EventDispatcher.indexOfListener(listeners,scope,callBack);
      if(index != -1)
      {
         listeners.splice(index,1);
      }
   }
   function dispatchEvent(event)
   {
      if(event.type == "all")
      {
         return undefined;
      }
      if(event.target == undefined)
      {
         event.target = this;
      }
      this.dispatchQueue(this,event);
   }
   function hasEventListener(event)
   {
      return this._listeners[event] != null && this._listeners[event].length > 0;
   }
   function removeAllEventListeners(event)
   {
      if(event == undefined)
      {
         delete this._listeners;
      }
      else
      {
         delete this._listeners[event];
      }
   }
   function dispatchQueue(dispatch, event)
   {
      var listeners = dispatch._listeners[event.type];
      if(listeners != undefined)
      {
         gfx.events.EventDispatcher.$dispatchEvent(dispatch,listeners,event);
      }
      listeners = dispatch._listeners.all;
      if(listeners != undefined)
      {
         gfx.events.EventDispatcher.$dispatchEvent(dispatch,listeners,event);
      }
   }
   static function $dispatchEvent(dispatch, listeners, event)
   {
      var l = listeners.length;
      var i = 0;
      while(i < l)
      {
         var listenerObject = listeners[i].listenerObject;
         var listenerType = typeof listenerObject;
         var listenerFunction = listeners[i].listenerFunction;
         if(listenerFunction == undefined)
         {
            listenerFunction = event.type;
         }
         if(listenerType != "function")
         {
            if(listenerObject.handleEvent != undefined && listenerFunction == undefined)
            {
               listenerObject.handleEvent(event);
            }
            else
            {
               listenerObject[listenerFunction](event);
            }
         }
         else if(listenerObject[listenerFunction] != null)
         {
            listenerObject[listenerFunction](event);
         }
         else
         {
            listenerObject.apply(dispatch,[event]);
         }
         i++;
      }
   }
   function cleanUp()
   {
      this.cleanUpEvents();
   }
   function cleanUpEvents()
   {
      this.removeAllEventListeners();
   }
}
