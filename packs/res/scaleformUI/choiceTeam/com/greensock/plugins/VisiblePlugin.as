class com.greensock.plugins.VisiblePlugin extends com.greensock.plugins.TweenPlugin
{
   var _target;
   var _tween;
   var _progress;
   var _initVal;
   var _visible;
   static var API = 2;
   function VisiblePlugin()
   {
      super("_visible");
   }
   function _onInitTween(target, value, tween)
   {
      this._target = target;
      this._tween = tween;
      this._progress = !this._tween.vars.runBackwards ? 1 : 0;
      this._initVal = this._target._visible;
      this._visible = Boolean(value);
      return true;
   }
   function setRatio(v)
   {
      this._target._visible = !(v == 1 && (this._tween._time / this._tween._duration == this._progress || this._tween._duration == 0)) ? this._initVal : this._visible;
   }
}
