class com.greensock.plugins.FilterPlugin extends com.greensock.plugins.TweenPlugin
{
   var _target;
   var _tween;
   var _type;
   var _index;
   var _filter;
   var _remove;
   var _propName;
   static var API = 2;
   function FilterPlugin(props, priority)
   {
      super(props,priority);
   }
   function _initFilter(target, props, tween, type, defaultFilter, propNames)
   {
      this._target = target;
      this._tween = tween;
      this._type = type;
      var filters = this._target.filters;
      var p;
      var i;
      var colorTween;
      var extras = !(props instanceof flash.filters.BitmapFilter) ? props : {};
      if(extras.index != null)
      {
         this._index = extras.index;
      }
      else
      {
         this._index = filters.length;
         if(extras.addFilter != true)
         {
            while(--this._index > -1 && !(filters[this._index] instanceof this._type))
            {
            }
         }
      }
      if(this._index < 0 || !(filters[this._index] instanceof this._type))
      {
         if(this._index < 0)
         {
            this._index = filters.length;
         }
         if(this._index > filters.length)
         {
            i = filters.length - 1;
            while(++i < this._index)
            {
               filters[i] = new flash.filters.BlurFilter(0,0,1);
            }
         }
         filters[this._index] = defaultFilter;
         this._target.filters = filters;
      }
      this._filter = filters[this._index];
      this._remove = extras.remove == true;
      i = propNames.length;
      while(--i > -1)
      {
         p = propNames[i];
         if(props[p] != null && this._filter[p] != props[p])
         {
            if(p == "color" || p == "highlightColor" || p == "shadowColor")
            {
               colorTween = new com.greensock.plugins.HexColorsPlugin();
               colorTween._initColor(this._filter,p,props[p]);
               this._addTween(colorTween,"setRatio",0,1,this._propName);
            }
            else if(p == "quality" || p == "inner" || p == "knockout" || p == "hideObject")
            {
               this._filter[p] = props[p];
            }
            else
            {
               this._addTween(this._filter,p,this._filter[p],props[p],this._propName);
            }
         }
      }
      return true;
   }
   function setRatio(v)
   {
      super.setRatio(v);
      var filters = this._target.filters;
      if(!(filters[this._index] instanceof this._type))
      {
         this._index = filters.length;
         while(--this._index > -1 && !(filters[this._index] instanceof this._type))
         {
         }
         if(this._index == -1)
         {
            this._index = filters.length;
         }
      }
      if(v == 1 && this._remove && this._tween._time == this._tween._duration)
      {
         if(this._index < filters.length)
         {
            filters.splice(this._index,1);
         }
      }
      else
      {
         filters[this._index] = this._filter;
      }
      this._target.filters = filters;
   }
}
