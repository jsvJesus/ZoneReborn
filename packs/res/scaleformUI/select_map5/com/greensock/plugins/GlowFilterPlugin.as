class com.greensock.plugins.GlowFilterPlugin extends com.greensock.plugins.FilterPlugin
{
   static var API = 2;
   static var _propNames = ["color","alpha","blurX","blurY","strength","quality","inner","knockout"];
   function GlowFilterPlugin()
   {
      super("glowFilter");
   }
   function _onInitTween(target, value, tween)
   {
      return this._initFilter(target,value,tween,flash.filters.GlowFilter,new flash.filters.GlowFilter(16777215,0,0,0,value.strength || 1,value.quality || 2,value.inner,value.knockout),com.greensock.plugins.GlowFilterPlugin._propNames);
   }
}
