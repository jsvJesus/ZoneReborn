class com.greensock.plugins.TweenPlugin
{
   var _overwriteProps;
   var _propName;
   var _priority;
   var _firstPT;
   static var version = "12.0.14";
   static var API = 2;
   function TweenPlugin(props, priority)
   {
      this._overwriteProps = props.split(",");
      this._propName = this._overwriteProps[0];
      this._priority = priority || 0;
   }
   function _onInitTween(target, value, tween)
   {
      return false;
   }
   function _addTween(target, propName, start, end, overwriteProp, round)
   {
      var c;
      if(end != null && (c = !(typeof end == "number" || end.charAt(1) !== "=") ? Number(end.charAt(0) + "1") * Number(end.substr(2)) : Number(end) - start))
      {
         this._firstPT = {_next:this._firstPT,t:target,p:propName,s:start,c:c,f:typeof target[propName] == "function",n:overwriteProp || propName,r:round};
         if(this._firstPT._next)
         {
            this._firstPT._next._prev = this._firstPT;
         }
         return this._firstPT;
      }
      return null;
   }
   function setRatio(v)
   {
      var pt = this._firstPT;
      var val;
      while(pt)
      {
         val = pt.c * v + pt.s;
         if(pt.r)
         {
            val = val + (val <= 0 ? -0.5 : 0.5) | 0;
         }
         if(pt.f)
         {
            pt.t[pt.p](val);
         }
         else
         {
            pt.t[pt.p] = val;
         }
         pt = pt._next;
      }
   }
   function _kill(lookup)
   {
      if(lookup[this._propName] != null)
      {
         this._overwriteProps = [];
      }
      else
      {
         var i = this._overwriteProps.length;
         while(--i > -1)
         {
            if(lookup[this._overwriteProps[i]] != null)
            {
               this._overwriteProps.splice(i,1);
            }
         }
      }
      var pt = this._firstPT;
      while(pt)
      {
         if(lookup[pt.n] != null)
         {
            if(pt._next)
            {
               pt._next._prev = pt._prev;
            }
            if(pt._prev)
            {
               pt._prev._next = pt._next;
               pt._prev = null;
            }
            else if(this._firstPT == pt)
            {
               this._firstPT = pt._next;
            }
         }
         pt = pt._next;
      }
      return false;
   }
   function _roundProps(lookup, value)
   {
      var pt = this._firstPT;
      while(pt)
      {
         if(lookup[this._propName] || pt.n != null && lookup[pt.n.split(this._propName + "_").join("")])
         {
            pt.r = value;
         }
         pt = pt._next;
      }
   }
   static function _onTweenEvent(type, tween)
   {
      var pt = tween._firstPT;
      var changed;
      if(type === "_onInitAllProps")
      {
         var pt2;
         var first;
         var last;
         var next;
         while(pt)
         {
            next = pt._next;
            pt2 = first;
            while(pt2 && pt2.pr > pt.pr)
            {
               pt2 = pt2._next;
            }
            if(pt._prev = !pt2 ? last : pt2._prev)
            {
               pt._prev._next = pt;
            }
            else
            {
               first = pt;
            }
            if(pt._next = pt2)
            {
               pt2._prev = pt;
            }
            else
            {
               last = pt;
            }
            pt = next;
         }
         pt = tween._firstPT = first;
      }
      while(pt)
      {
         if(pt.pg)
         {
            if(typeof pt.t[type] === "function")
            {
               if(pt.t[type]())
               {
                  changed = true;
               }
            }
         }
         pt = pt._next;
      }
      return changed;
   }
   static function activate(plugins)
   {
      com.greensock.TweenLite._onPluginEvent = com.greensock.plugins.TweenPlugin._onTweenEvent;
      var i = plugins.length;
      while(--i > -1)
      {
         if(plugins[i].API == com.greensock.plugins.TweenPlugin.API)
         {
            com.greensock.TweenLite._plugins[new plugins[i]()._propName] = plugins[i];
         }
      }
      return true;
   }
}
