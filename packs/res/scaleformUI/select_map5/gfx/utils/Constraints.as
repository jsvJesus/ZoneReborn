class gfx.utils.Constraints
{
   var scope;
   var elements;
   static var LEFT = 1;
   static var RIGHT = 2;
   static var TOP = 4;
   static var BOTTOM = 8;
   static var ALL = gfx.utils.Constraints.LEFT | gfx.utils.Constraints.RIGHT | gfx.utils.Constraints.TOP | gfx.utils.Constraints.BOTTOM;
   var scaled = false;
   function Constraints(scope, scaled)
   {
      this.scope = scope;
      this.scaled = scaled;
      this.elements = [];
   }
   function addElement(clip, edges)
   {
      if(clip == null)
      {
         return undefined;
      }
      var xAdjust = 100 / this.scope._xscale;
      var yAdjust = 100 / this.scope._yscale;
      var w = this.scope._width;
      var h = this.scope._height;
      if(this.scope == _root)
      {
         w = Stage.width;
         h = Stage.height;
      }
      var element = {clip:clip,edges:edges,metrics:{left:clip._x,top:clip._y,right:w * xAdjust - (clip._x + clip._width),bottom:h * yAdjust - (clip._y + clip._height),xscale:clip._xscale,yscale:clip._yscale}};
      var m = element.metrics;
      this.elements.push(element);
   }
   function removeElement(clip)
   {
      var i = 0;
      while(i < this.elements.length)
      {
         if(this.elements[i].clip == clip)
         {
            this.elements.splice(i,1);
            break;
         }
         i++;
      }
   }
   function getElement(clip)
   {
      var i = 0;
      while(i < this.elements.length)
      {
         if(this.elements[i].clip == clip)
         {
            return this.elements[i];
         }
         i++;
      }
      return null;
   }
   function update(width, height)
   {
      var xAdjust = 100 / this.scope._xscale;
      var yAdjust = 100 / this.scope._yscale;
      if(!this.scaled)
      {
         this.scope._xscale = 100;
         this.scope._yscale = 100;
      }
      var i = 0;
      while(i < this.elements.length)
      {
         var element = this.elements[i];
         var edges = element.edges;
         var clip = element.clip;
         var metrics = element.metrics;
         var w = clip.width == null ? "_width" : "width";
         var h = clip.height == null ? "_height" : "height";
         if(this.scaled)
         {
            clip._xscale = metrics.xscale * xAdjust;
            clip._yscale = metrics.yscale * yAdjust;
            if((edges & gfx.utils.Constraints.LEFT) > 0)
            {
               clip._x = metrics.left * xAdjust;
               if((edges & gfx.utils.Constraints.RIGHT) > 0)
               {
                  var nw = width - metrics.left - metrics.right;
                  if(!(clip instanceof TextField))
                  {
                     nw *= xAdjust;
                  }
                  clip[w] = nw;
               }
            }
            else if((edges & gfx.utils.Constraints.RIGHT) > 0)
            {
               clip._x = (width - metrics.right) * xAdjust - clip._width;
            }
            if((edges & gfx.utils.Constraints.TOP) > 0)
            {
               clip._y = metrics.top * yAdjust;
               if((edges & gfx.utils.Constraints.BOTTOM) > 0)
               {
                  var nh = height - metrics.top - metrics.bottom;
                  if(!(clip instanceof TextField))
                  {
                     nh *= yAdjust;
                  }
                  clip[h] = nh;
               }
            }
            else if((edges & gfx.utils.Constraints.BOTTOM) > 0)
            {
               clip._y = (height - metrics.bottom) * yAdjust - clip._height;
            }
         }
         else
         {
            if((edges & gfx.utils.Constraints.RIGHT) > 0)
            {
               if((edges & gfx.utils.Constraints.LEFT) > 0)
               {
                  clip[w] = width - metrics.left - metrics.right;
               }
               else
               {
                  clip._x = width - clip._width - metrics.right;
               }
            }
            if((edges & gfx.utils.Constraints.BOTTOM) > 0)
            {
               if((edges & gfx.utils.Constraints.TOP) > 0)
               {
                  clip[h] = height - metrics.top - metrics.bottom;
               }
               else
               {
                  clip._y = height - clip._height - metrics.bottom;
               }
            }
         }
         i++;
      }
   }
   function toString()
   {
      return "[Scaleform Constraints]";
   }
}
