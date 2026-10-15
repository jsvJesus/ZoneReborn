class com.greensock.plugins.FramePlugin extends com.greensock.plugins.TweenPlugin
{
   var _target;
   var frame;
   static var API = 2;
   function FramePlugin()
   {
      super("frame,frameLabel,frameForward,frameBackward");
   }
   function _onInitTween(target, value, tween)
   {
      if(typeof target != "movieclip" || isNaN(value))
      {
         return false;
      }
      this._target = MovieClip(target);
      this.frame = this._target._currentframe;
      this._addTween(this,"frame",this.frame,value,"frame",true);
      return true;
   }
   function setRatio(v)
   {
      super.setRatio(v);
      if(this.frame != this._target._currentframe)
      {
         this._target.gotoAndStop(this.frame);
      }
   }
}
