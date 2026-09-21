class mx.events.EventDispatcher
{
   static var _fEventDispatcher = undefined;
   static var exceptions = {move:1,draw:1,load:1};
   function EventDispatcher()
   {
   }
   static function _removeEventListener(queue, event, handler)
   {
      if(queue != undefined)
      {
         var l = queue.length;
         var i;
         i = 0;
         while(i < l)
         {
            var o = queue[i];
            if(o == handler)
            {
               queue.splice(i,1);
               return undefined;
            }
            i++;
         }
      }
   }
   static function initialize(object)
   {
      if(mx.events.EventDispatcher._fEventDispatcher == undefined)
      {
         mx.events.EventDispatcher._fEventDispatcher = new mx.events.EventDispatcher();
      }
      object.addEventListener = mx.events.EventDispatcher._fEventDispatcher.addEventListener;
      object.removeEventListener = mx.events.EventDispatcher._fEventDispatcher.removeEventListener;
      object.dispatchEvent = mx.events.EventDispatcher._fEventDispatcher.dispatchEvent;
      object.dispatchQueue = mx.events.EventDispatcher._fEventDispatcher.dispatchQueue;
   }
   function dispatchQueue(queueObj, eventObj)
   {
      var queueName = "__q_" + eventObj.type;
      var queue = queueObj[queueName];
      if(queue != undefined)
      {
         var i;
         for(i in queue)
         {
            var o = queue[i];
            var oType = typeof o;
            if(oType == "object" || oType == "movieclip")
            {
               if(o.handleEvent != undefined)
               {
                  o.handleEvent(eventObj);
               }
               if(o[eventObj.type] != undefined)
               {
                  if(mx.events.EventDispatcher.exceptions[eventObj.type] == undefined)
                  {
                     o[eventObj.type](eventObj);
                  }
               }
            }
            else
            {
               o.apply(queueObj,[eventObj]);
            }
         }
      }
   }
   function dispatchEvent(eventObj)
   {
      if(eventObj.target == undefined)
      {
         eventObj.target = this;
      }
      this[eventObj.type + "Handler"](eventObj);
      this.dispatchQueue(this,eventObj);
   }
   function addEventListener(event, handler)
   {
      var queueName = "__q_" + event;
      if(this[queueName] == undefined)
      {
         this[queueName] = new Array();
      }
      _global.ASSetPropFlags(this,queueName,1);
      mx.events.EventDispatcher._removeEventListener(this[queueName],event,handler);
      this[queueName].push(handler);
   }
   function removeEventListener(event, handler)
   {
      var queueName = "__q_" + event;
      mx.events.EventDispatcher._removeEventListener(this[queueName],event,handler);
   }
}
