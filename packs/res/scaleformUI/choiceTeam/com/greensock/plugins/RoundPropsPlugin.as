class com.greensock.plugins.RoundPropsPlugin extends com.greensock.plugins.TweenPlugin
{
   var _tween;
   var _overwriteProps;
   static var API = 2;
   function RoundPropsPlugin()
   {
      super("roundProps",-1);
   }
   function _onInitTween(target, value, tween)
   {
      this._tween = tween;
      return true;
   }
   function _onInitAllProps()
   {
      var rp = !(this._tween.vars.roundProps instanceof Array) ? this._tween.vars.roundProps.split(",") : this._tween.vars.roundProps;
      var i = rp.length;
      var lookup = {};
      var rpt = this._tween._propLookup.roundProps;
      var prop;
      var pt;
      var next;
      while(--i > -1)
      {
         lookup[rp[i]] = 1;
      }
      i = rp.length;
      while(--i > -1)
      {
         prop = rp[i];
         pt = this._tween._firstPT;
         while(pt)
         {
            next = pt._next;
            if(pt.pg)
            {
               pt.t._roundProps(lookup,true);
            }
            else if(pt.n == prop)
            {
               this._add(pt.t,prop,pt.s,pt.c);
               if(next)
               {
                  next._prev = pt._prev;
               }
               if(pt._prev)
               {
                  pt._prev._next = next;
               }
               else if(this._tween._firstPT == pt)
               {
                  this._tween._firstPT = next;
               }
               pt._next = pt._prev = null;
               this._tween._propLookup[prop] = rpt;
            }
            pt = next;
         }
      }
      return false;
   }
   function _add(target, p, s, c)
   {
      this._addTween(target,p,s,s + c,p,true);
      this._overwriteProps[this._overwriteProps.length] = p;
   }
}
