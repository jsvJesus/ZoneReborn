class com.greensock.plugins.FrameLabelPlugin extends com.greensock.plugins.FramePlugin
{
   var _propName;
   var _target;
   var frame;
   static var API = 2;
   function FrameLabelPlugin()
   {
      super();
      this._propName = "frameLabel";
   }
   function _onInitTween(target, value, tween)
   {
      if(typeof tween.target != "movieclip")
      {
         return false;
      }
      this._target = MovieClip(target);
      this.frame = this._target._currentframe;
      var mc = this._target.duplicateMovieClip("__frameLabelPluginTempMC",this._target._parent.getNextHighestDepth());
      mc.gotoAndStop(value);
      var endFrame = mc._currentframe;
      mc.removeMovieClip();
      if(this.frame != endFrame)
      {
         this._addTween(this,"frame",this.frame,endFrame,"frame",true);
      }
      return true;
   }
}
