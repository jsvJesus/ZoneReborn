class com.greensock.plugins.BlurFilterPlugin extends com.greensock.plugins.FilterPlugin
{
   static var API = 2;
   static var _propNames = ["blurX","blurY","quality"];
   function BlurFilterPlugin()
   {
      super("blurFilter");
   }
   function _onInitTween(target, value, tween)
   {
      return this._initFilter(target,value,tween,flash.filters.BlurFilter,new flash.filters.BlurFilter(0,0,value.quality || 2),com.greensock.plugins.BlurFilterPlugin._propNames);
   }
}
