package ui.screens
{
   import com.dvalimona.components.*;
   import communication.*;
   import events.*;
   import flash.events.*;
   import flash.utils.*;
   import lang.*;
   import logging.*;
   import ui.Screen;
   import ui.components.*;
   
   public class NewCharScreen extends Screen
   {
      protected var vBox:VBox;
      
      protected var linesBox:VBox;
      
      protected var headerPanel:HeaderPanel;
      
      protected var createButton:MenuButton;
      
      protected var backButton:MenuButton;
      
      protected var charNameBox:HBoxLine;
      
      protected var charNameLabel:Label;
      
      public var charNameInput:InputText;
      
      protected var randomButton:MenuButton;
      
      protected var itemSteppers:Array;
      
      protected var quad:Quad;
      
      protected var isFirstChar:Boolean;
      
      protected var nameAllowed:Boolean;
      
      public function NewCharScreen(id:String, depth:uint = 0)
      {
         super(id,depth,false);
      }
      
      public function setFirstChar(value:Boolean) : *
      {
         this.isFirstChar = value;
         if(this.isFirstChar)
         {
            this.backButton.$ = "extendedGUI.RootWindow.quitButton";
         }
         else
         {
            this.backButton.$ = "extendedGUI.NewCharWindow.backButton";
         }
      }
      
      override protected function unfreeze(... args) : void
      {
         Dummy.hide();
         this.label = BreadCrumbs.DIV + Locale.getById("extendedGUI.NewCharWindow.newChar");
         super.unfreeze();
         this.clearLinesBox();
         this.charNameInput.text = "";
         this.nameAllowed = false;
         this.updateCreateButton();
         this.charNameBox.draw();
         this.linesBox.draw();
         this.vBox.draw();
         Logger.LogToChannel(Logger.DEBUG,"NewCharScreen.unfreeze");
         setTimeout(Character.StartCharCreating,50,this.onConfigReceived);
      }
      
      override protected function init(... args) : void
      {
         this.vBox = new VBox();
         this.vBox.x = 50;
         this.vBox.y = 100;
         this.vBox.alignment = VBox.LEFT;
         this.vBox.spacing = 1;
         this.vBox.debug = false;
         super.addChild(this.vBox);
         this.charNameBox = new HBoxLine();
         this.charNameBox.paddingLeft = 0;
         this.charNameBox.setSize(widths[0],40);
         this.charNameLabel = new Label(this.charNameBox.left);
         this.charNameLabel.autoSize = true;
         this.charNameLabel.$ = "extendedGUI.NewCharWindow.charName";
         this.charNameLabel.y = 9;
         this.charNameLabel.size = 20;
         this.charNameLabel.paddingLeft = 10;
         this.charNameInput = new InputText(this.charNameBox.right);
         this.charNameInput.setSize(300,40);
         this.charNameInput.size = 22;
         this.charNameInput.paddingRight = 0;
         this.charNameInput.addEventListener(Event.CHANGE,this.onCharNameChanged);
         this.charNameInput.maxChars = 32;
         this.vBox.addChild(this.charNameBox);
         this.linesBox = new VBox();
         this.linesBox.alignment = VBox.LEFT;
         this.linesBox.debug = false;
         this.linesBox.width = widths[0];
         this.linesBox.spacing = 1;
         this.linesBox.paddingLeft = 0;
         this.linesBox.paddingTop = 10;
         this.vBox.addChild(this.linesBox);
         this.randomButton = new MenuButton(this.vBox);
         this.randomButton.$ = "extendedGUI.NewCharWindow.randomizeChar";
         this.randomButton.height = 35;
         this.randomButton.paddingTop = 10;
         this.randomButton.addEventListener(MouseEvent.CLICK,this.randomClickHandler);
         this.createButton = new MenuButton(this.vBox,0,0,"",this.onCreateButtonHandler);
         this.createButton.$ = "extendedGUI.NewCharWindow.goCreate";
         this.createButton.height = 35;
         this.createButton.enabled = false;
         this.backButton = new MenuButton(this.vBox,0,0,"",this.onBackButtonHandler);
         this.backButton.$ = "extendedGUI.NewCharWindow.backButton";
         this.backButton.height = 35;
         appears = [];
         this.charNameInput.text = "";
         this.defaultFocus = this.charNameInput;
      }
      
      protected function setFocus() : void
      {
         Base.stage.focus = this.charNameInput.textField;
      }
      
      protected function onCharNameChanged(event:Event) : void
      {
         Logger.LogToChannel(Logger.DEBUG,"NewCharScreen.onCharNameChanged");
         try
         {
            this.nameAllowed = false;
            this.updateCreateButton();
            event.stopPropagation();
            Callout.ClearInstances();
            Api.self.removeEventListener(Api.CHECK_AVATAR_NAME,this.onCheckNameHandler);
            Api.self.addEventListener(Api.CHECK_AVATAR_NAME,this.onCheckNameHandler);
            Api.call(Api.CHECK_AVATAR_NAME,[{"nick":this.charNameInput.text}]);
         }
         catch(error:Error)
         {
            Logger.LogToChannel(Logger.ERROR,error);
         }
      }
      
      protected function onCheckNameHandler(event:ApiEvent) : void
      {
         Api.self.removeEventListener(Api.CHECK_AVATAR_NAME,this.onCheckNameHandler);
         Logger.LogToChannel(Logger.DEBUG,"NewCharScreen.onCheckNameHandler",event.data.answer,event.data.error,event.data.answer.result);
         if(event.data.answer.result)
         {
            this.nameAllowed = true;
         }
         else
         {
            this.nameAllowed = false;
         }
         this.showNameCallout(event.data.answer.message);
         this.updateCreateButton();
      }
      
      private function updateCreateButton() : void
      {
         Logger.LogToChannel(Logger.DEBUG,"NewCharScreen.updateCreateButton",this.charNameInput.text.length > 0,this.nameAllowed);
         this.createButton.enabled = this.charNameInput.text.length > 0 && Boolean(this.nameAllowed);
      }
      
      public function createButtonEnabled(value:Boolean) : void
      {
         this.createButton.enabled = value;
         this.nameAllowed = value;
      }
      
      private function showNameCallout(text:String) : void
      {
         var callout:Callout = new Callout(this.charNameInput,text,300,50);
      }
      
      protected function randomClickHandler(event:MouseEvent) : void
      {
         this.randomize();
      }
      
      protected function randomize() : void
      {
         var item:ItemStepper = null;
         var data:Array = new Array();
         for(var i:uint = 0; i < this.itemSteppers.length; i++)
         {
            item = this.itemSteppers[i] as ItemStepper;
            item.randomize(true);
            data.push({
               "var":item.currentItem,
               "choiceGroup":item.itemGroup
            });
         }
         Api.call(Api.NEW_FULL_CHAR_VIEW,[data]);
      }
      
      protected function onCreateButtonHandler(event:Event) : void
      {
         this.setFirstChar(false);
         Api.self.addEventListener(Api.CREATE_CHAR,this.onCreateCharHandler);
         Api.call(Api.CREATE_CHAR,[]);
      }
      
      protected function onCreateCharHandler(event:ApiEvent) : void
      {
         if(event.data.answer.success == 1)
         {
            this.dispatchEvent(new ScreenEvent(ScreenEvent.GO_SCREEN,MainMenuGUI.ROOT_SCREEN));
            setTimeout(Character.Update,1500);
         }
         else
         {
            Base.navigator.showDialog("extendedGUI.Dialogs.Error",event.data.answer.msg,true,[new DialogButtonItem("extendedGUI.Dialogs.Ok",null,1)]);
         }
      }
      
      protected function onBackButtonHandler(event:Event) : void
      {
         this.goBack();
         this.setFirstChar(false);
      }
      
      override public function goBack() : void
      {
         Character.CancelCharCreating();
         if(this.isFirstChar)
         {
            Auth.self.doLogout();
         }
         else
         {
            this.dispatchEvent(new ScreenEvent(ScreenEvent.GO_SCREEN,MainMenuGUI.CHAR_SCREEN));
         }
         BreadCrumbs.Remove(this.breadCrumb);
      }
      
      private function cancelCharCreating() : void
      {
         Character.CancelCharCreating();
      }
      
      private function clearLinesBox() : void
      {
         while(this.linesBox.numChildren > 0)
         {
            this.linesBox.removeChildAt(0);
         }
      }
      
      private function onConfigReceived(data:Object) : void
      {
         var inset:Object = null;
         var part:Object = null;
         var items:Array = null;
         var itemStepper:ItemStepper = null;
         var i:String = null;
         var p:String = null;
         Logger.LogToChannel(Logger.DEBUG,"NewCharScreen.onConfigReceived",data);
         this.clearLinesBox();
         this.itemSteppers = new Array();
         var cnt:uint = 0;
         for(i in data)
         {
            items = new Array();
            inset = data[i];
            Logger.LogToChannel(Logger.DEBUG,"i:",i);
            for(p in inset)
            {
               part = inset[p];
               part.ranger = parseInt(p);
               items.push(part);
            }
            itemStepper = new ItemStepper();
            itemStepper.useDiagridBack = true;
            itemStepper.itemGroup = i;
            itemStepper.items = items;
            itemStepper.reverseItems();
            itemStepper.width = widths[0] - 0;
            itemStepper.height = 35;
            itemStepper.addEventListener(Event.CHANGE,this.onItemsChange);
            this.linesBox.addChild(itemStepper);
            this.itemSteppers.push(itemStepper);
            cnt++;
         }
         this.charNameBox.draw();
         this.linesBox.draw();
         this.vBox.draw();
         this.randomize();
         setTimeout(this.setFocus,0);
         setTimeout(Dummy.show,30);
      }
      
      protected function onItemsChange(event:Event) : void
      {
         Logger.LogToChannel(Logger.DEBUG,"onItemsChange:",event.target as ItemStepper);
         Api.call(Api.NEW_CHAR_VIEW,[{
            "var":(event.target as ItemStepper).currentItem,
            "choiceGroup":(event.target as ItemStepper).itemGroup
         }]);
      }
   }
}

