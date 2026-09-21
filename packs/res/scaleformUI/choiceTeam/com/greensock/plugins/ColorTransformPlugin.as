class com.greensock.plugins.ColorTransformPlugin extends com.greensock.plugins.TintPlugin
{
   var _propName;
   static var API = 2;
   function ColorTransformPlugin()
   {
      super();
      this._propName = "colorTransform";
   }
   function _onInitTween(target, value, tween)
   {
      if(typeof target != "movieclip" && !(target instanceof TextField))
      {
         return false;
      }
      var color = new Color(target);
      var end = color.getTransform();
      if(value.redMultiplier != null)
      {
         end.ra = value.redMultiplier * 100;
      }
      if(value.greenMultiplier != null)
      {
         end.ga = value.greenMultiplier * 100;
      }
      if(value.blueMultiplier != null)
      {
         end.ba = value.blueMultiplier * 100;
      }
      if(value.alphaMultiplier != null)
      {
         end.aa = value.alphaMultiplier * 100;
      }
      if(value.redOffset != null)
      {
         end.rb = value.redOffset;
      }
      if(value.greenOffset != null)
      {
         end.gb = value.greenOffset;
      }
      if(value.blueOffset != null)
      {
         end.bb = value.blueOffset;
      }
      if(value.alphaOffset != null)
      {
         end.ab = value.alphaOffset;
      }
      if(!isNaN(value.tint) || !isNaN(value.color))
      {
         var tint = !!isNaN(value.tint) ? value.color : value.tint;
         if(tint != null)
         {
            end.rb = Number(tint) >> 16;
            end.gb = Number(tint) >> 8 & 0xFF;
            end.bb = Number(tint) & 0xFF;
            end.ra = 0;
            end.ga = 0;
            end.ba = 0;
         }
      }
      if(!isNaN(value.tintAmount))
      {
         var ratio = value.tintAmount / (1 - (end.ra + end.ga + end.ba) / 300);
         end.rb *= ratio;
         end.gb *= ratio;
         end.bb *= ratio;
         end.ra = end.ga = end.ba = (1 - value.tintAmount) * 100;
      }
      else if(!isNaN(value.exposure))
      {
         end.rb = end.gb = end.bb = 255 * (value.exposure - 1);
         end.ra = end.ga = end.ba = 100;
      }
      else if(!isNaN(value.brightness))
      {
         end.rb = end.gb = end.bb = Math.max(0,(value.brightness - 1) * 255);
         end.ra = end.ga = end.ba = (1 - Math.abs(value.brightness - 1)) * 100;
      }
      if(tween.vars._alpha != null && value.alphaMultiplier == null)
      {
         end.aa = tween.vars._alpha;
         tween._kill({_alpha:1});
      }
      this._init(target,end);
      return true;
   }
}
