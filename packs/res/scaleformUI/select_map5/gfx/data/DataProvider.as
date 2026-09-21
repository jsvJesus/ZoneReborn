class gfx.data.DataProvider extends Array
{
   var dispatchEvent;
   var cleanUpEvents;
   static var instance;
   var isDataProvider = true;
   function DataProvider(total)
   {
      super();
      gfx.events.EventDispatcher.initialize(this);
   }
   static function initialize(data)
   {
      if(gfx.data.DataProvider.instance == undefined)
      {
         gfx.data.DataProvider.instance = new gfx.data.DataProvider();
      }
      var members = ["indexOf","requestItemAt","requestItemRange","invalidate","toString","cleanUp","isDataProvider"];
      var i = 0;
      while(i < members.length)
      {
         data[members[i]] = gfx.data.DataProvider.instance[members[i]];
         i++;
      }
      gfx.events.EventDispatcher.initialize(data);
      _global.ASSetPropFlags(data,members,1);
      _global.ASSetPropFlags(data,"addEventListener,removeEventListener,hasEventListener,removeAllEventListeners,dispatchEvent,dispatchQueue,cleanUpEvents",1);
   }
   function indexOf(value, scope, callBack)
   {
      var i = 0;
      i = 0;
      while(i < this.length)
      {
         if(this[i] == value)
         {
            break;
         }
         i++;
      }
      var index = i != this.length ? i : -1;
      if(callBack)
      {
         scope[callBack].call(scope,index);
      }
      return index;
   }
   function requestItemAt(index, scope, callBack)
   {
      var item = this[index];
      if(callBack)
      {
         scope[callBack].call(scope,item);
      }
      return item;
   }
   function requestItemRange(startIndex, endIndex, scope, callBack)
   {
      var items = this.slice(startIndex,endIndex + 1);
      if(callBack)
      {
         scope[callBack].call(scope,items);
      }
      return items;
   }
   function invalidate(length)
   {
      this.dispatchEvent({type:"change"});
   }
   function cleanUp()
   {
      this.splice(0,this.length);
      this.cleanUpEvents();
   }
   function toString()
   {
      return "[DataProvider (" + this.length + ")]";
   }
}
