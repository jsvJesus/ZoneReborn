class com.greensock.plugins.DropShadowFilterPlugin extends com.greensock.plugins.FilterPlugin
{
   static var API = 2;
   static var _propNames = ["distance","angle","color","alpha","blurX","blurY","strength","quality","inner","knockout","hideObject"];
   function DropShadowFilterPlugin()
   {
      super("dropShadowFilter");
   }
   function _onInitTween(target, value, tween)
   {
      return this._initFilter(target,value,tween,flash.filters.DropShadowFilter,new flash.filters.DropShadowFilter(0,45,0,0,0,0,1,value.quality || 2,value.inner,value.knockout,value.hideObject),com.greensock.plugins.DropShadowFilterPlugin._propNames);
   }
}
