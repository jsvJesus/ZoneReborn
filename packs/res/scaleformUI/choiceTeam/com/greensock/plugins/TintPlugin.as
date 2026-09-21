class com.greensock.plugins.TintPlugin extends com.greensock.plugins.TweenPlugin
{
   var _color;
   var _firstPT;
   static var API = 2;
   function TintPlugin()
   {
      super("tint,colorTransform,removeTint");
   }
   function _onInitTween(target, value, tween)
   {
      if(typeof target != "movieclip" && !(target instanceof TextField))
      {
         return false;
      }
      var alpha = tween.vars._alpha == undefined ? (tween.vars.autoAlpha == null ? target._alpha : tween.vars.autoAlpha) : tween.vars._alpha;
      var n = Number(value);
      var end = !(value == null || tween.vars.removeTint == true) ? {rb:n >> 16,gb:n >> 8 & 0xFF,bb:n & 0xFF,ra:0,ga:0,ba:0,aa:alpha} : {rb:0,gb:0,bb:0,ab:0,ra:alpha,ga:alpha,ba:alpha,aa:alpha};
      this._init(target,end);
      return true;
   }
   function _init(target, end)
   {
      this._color = new Color(target);
      var ct = this._color.getTransform();
      for(var p in end)
      {
         if(ct[p] != end[p])
         {
            this._addTween(ct,p,ct[p],end[p],"tint");
         }
      }
   }
   function setRatio(v)
   {
      var ct = this._color.getTransform();
      var pt = this._firstPT;
      while(pt)
      {
         ct[pt.p] = pt.c * v + pt.s;
         pt = pt._next;
      }
      this._color.setTransform(ct);
   }
}
