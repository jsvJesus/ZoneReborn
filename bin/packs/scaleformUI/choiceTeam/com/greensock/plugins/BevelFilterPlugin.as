class com.greensock.plugins.BevelFilterPlugin extends com.greensock.plugins.FilterPlugin
{
   static var API = 2;
   static var _propNames = ["distance","angle","highlightColor","highlightAlpha","shadowColor","shadowAlpha","blurX","blurY","strength","quality"];
   function BevelFilterPlugin()
   {
      super("bevelFilter");
   }
   function _onInitTween(target, value, tween)
   {
      return this._initFilter(target,value,tween,flash.filters.BevelFilter,new flash.filters.BevelFilter(0,0,16777215,0.5,0,0.5,2,2,0,value.quality || 2),com.greensock.plugins.BevelFilterPlugin._propNames);
   }
}
