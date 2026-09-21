class com.greensock.plugins.HexColorsPlugin extends com.greensock.plugins.TweenPlugin
{
   var _overwriteProps;
   var _colors;
   static var API = 2;
   function HexColorsPlugin()
   {
      super("hexColors");
      this._overwriteProps = [];
      this._colors = [];
   }
   function _onInitTween(target, value, tween)
   {
      for(var p in value)
      {
         this._initColor(target,p,Number(value[p]));
      }
      return true;
   }
   function _initColor(target, p, end)
   {
      var isFunc = typeof target[p] == "function";
      var start = !!isFunc ? target[!(p.indexOf("set") || typeof target["get" + p.substr(3)] !== "function") ? "get" + p.substr(3) : p]() : target[p];
      if(start != end)
      {
         var r = start >> 16;
         var g = start >> 8 & 0xFF;
         var b = start & 0xFF;
         this._colors[this._colors.length] = {t:target,p:p,f:isFunc,rs:r,rc:(end >> 16) - r,gs:g,gc:(end >> 8 & 0xFF) - g,bs:b,bc:(end & 0xFF) - b};
         this._overwriteProps[this._overwriteProps.length] = p;
      }
   }
   function _kill(lookup)
   {
      var i = this._colors.length;
      while(--i > -1)
      {
         if(lookup[this._colors[i].p] != null)
         {
            this._colors.splice(i,1);
         }
      }
      return super._kill(lookup);
   }
   function setRatio(v)
   {
      var i = this._colors.length;
      var clr;
      var val;
      while(--i > -1)
      {
         clr = this._colors[i];
         val = clr.rs + v * clr.rc << 16 | clr.gs + v * clr.gc << 8 | clr.bs + v * clr.bc;
         if(clr.f)
         {
            clr.t[clr.p](val);
         }
         else
         {
            clr.t[clr.p] = val;
         }
      }
   }
}
