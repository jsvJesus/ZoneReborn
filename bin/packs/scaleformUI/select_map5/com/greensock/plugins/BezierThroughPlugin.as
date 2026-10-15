class com.greensock.plugins.BezierThroughPlugin extends com.greensock.plugins.BezierPlugin
{
   var _propName;
   static var API = 2;
   function BezierThroughPlugin()
   {
      super();
      this._propName = "bezierThrough";
   }
   function _onInitTween(target, value, tween)
   {
      if(value instanceof Array)
      {
         value = {values:value};
      }
      value.type = "thru";
      return super._onInitTween(target,value,tween);
   }
}
