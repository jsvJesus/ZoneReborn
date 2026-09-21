class com.greensock.plugins.ShortRotationPlugin extends com.greensock.plugins.TweenPlugin
{
   var _overwriteProps;
   static var API = 2;
   function ShortRotationPlugin()
   {
      super("shortRotation");
      this._overwriteProps = [];
   }
   function _onInitTween(target, value, tween)
   {
      if(typeof value == "number")
      {
         return false;
      }
      var useRadians = Boolean(value.useRadians == true);
      var start;
      for(var p in value)
      {
         if(p != "useRadians")
         {
            start = typeof target[p] != "function" ? target[p] : target[!(p.indexOf("set") || typeof target["get" + p.substr(3)] !== "function") ? "get" + p.substr(3) : p]();
            this._initRotation(target,p,start,typeof value[p] != "number" ? start + Number(value[p].split("=").join("")) : Number(value[p]),useRadians);
         }
      }
      return true;
   }
   function _initRotation(target, p, start, end, useRadians)
   {
      var cap = !useRadians ? 360 : 6.283185307179586;
      var dif = (end - start) % cap;
      if(dif != dif % (cap / 2))
      {
         dif = dif >= 0 ? dif - cap : dif + cap;
      }
      this._addTween(target,p,start,start + dif,p);
      this._overwriteProps[this._overwriteProps.length] = p;
   }
}
