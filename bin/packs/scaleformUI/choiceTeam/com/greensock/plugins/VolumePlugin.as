class com.greensock.plugins.VolumePlugin extends com.greensock.plugins.TweenPlugin
{
   var _sound;
   var volume;
   static var API = 2;
   function VolumePlugin()
   {
      super("volume");
   }
   function _onInitTween(target, value, tween)
   {
      if(isNaN(value) || typeof target != "movieclip" && !(target instanceof Sound))
      {
         return false;
      }
      this._sound = typeof target != "movieclip" ? Sound(target) : new Sound(target);
      this.volume = this._sound.getVolume();
      this._addTween(this,"volume",this.volume,value,"volume");
      return true;
   }
   function setRatio(v)
   {
      super.setRatio(v);
      this._sound.setVolume(this.volume);
   }
}
