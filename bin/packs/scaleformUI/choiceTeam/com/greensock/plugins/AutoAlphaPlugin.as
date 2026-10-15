class com.greensock.plugins.AutoAlphaPlugin extends com.greensock.plugins.TweenPlugin
{
   var _target;
   var _ignoreVisible;
   static var API = 2;
   function AutoAlphaPlugin()
   {
      super("autoAlpha,_alpha,_visible");
   }
   function _onInitTween(target, value, tween)
   {
      this._addTween(this._target = target,"_alpha",target._alpha,value);
      return true;
   }
   function _kill(lookup)
   {
      this._ignoreVisible = lookup.hasOwnProperty("_visible");
      return super._kill(lookup);
   }
   function setRatio(v)
   {
      super.setRatio(v);
      if(!this._ignoreVisible)
      {
         this._target._visible = this._target._alpha != 0;
      }
   }
}
