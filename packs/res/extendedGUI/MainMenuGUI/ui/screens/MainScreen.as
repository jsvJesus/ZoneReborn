package ui.screens
{
   import com.dvalimona.components.*;
   import communication.*;
   import events.*;
   import flash.events.*;
   import flash.ui.*;
   import flash.utils.*;
   import lang.*;
   import logging.*;
   import ui.*;
   import ui.components.*;
   
   public class MainScreen extends Screen
   {
      private var vBox:VBox;
      
      private var charPanel:CharPanel;
      
      private var premium:NewPremiumPanel;
      
      private var settings:MenuButton;
      
      private var account:MenuButton;
      
      private var support:MenuButton;
      
      private var news:MenuButton;
      
      private var licenseBtn:MenuButton;
      
      private var quit:MenuButton;
      
      private var toGame:MenuButton;
      
      public function MainScreen(id:String, depth:uint = 0, use3D:Boolean = false)
      {
         Auth.self.addEventListener(Auth.AUTH_SUCCESS,this.onAuthSuccess);
         Character.core.addEventListener(Character.CHANGE,this.onCharacterChange);
         super(id,depth,use3D);
      }
      
      protected function onAuthSuccess(event:Event) : void
      {
         if(Base.navigator.currentScreen.id == this.id)
         {
            Dummy.show();
         }
      }
      
      override protected function unfreeze(... args) : void
      {
         setTimeout(this.unfreezeDeals,0);
      }
      
      private function unfreezeDeals() : void
      {
         this.label = Locale.getById("extendedGUI.RootWindow.caption");
         Character.Update();
         setTimeout(Base.navigator.header.invalidateGold,500);
         setTimeout(Base.navigator.header.invalidateGold,1000);
         super.unfreeze();
         setTimeout(this.updateToGameButton,100);
         setTimeout(this.updateToGameButton,500);
         setTimeout(this.updateToGameButton,1500);
         this.checkEULA();
         Dummy.visible = true;
         this.premium.updateView();
      }
      
      private function alignStartButton() : void
      {
      }
      
      private function checkEULA() : void
      {
         Logger.LogToChannel(Logger.DEBUG,"EULA.Status",EULA.Status);
         if(EULA.Status == -1)
         {
            Logger.LogToChannel(Logger.DEBUG,"EULA.Status == undefined");
            EULA.self.addEventListener(EULA.STATUS_EVENT,this.onEulaHandler);
            EULA.requestStatus();
         }
         else
         {
            this.EULADeals();
         }
      }
      
      private function EULADeals() : void
      {
         var eulaText:* = null;
         if(EULA.Status == 1)
         {
            Logger.LogToChannel(Logger.DEBUG,"EULA.Status == 1");
         }
         else if(EULA.Status == 0)
         {
            if(Navigator.NEWS_DIALOG != null)
            {
               Navigator.NEWS_DIALOG.doClose();
            }
            Logger.LogToChannel(Logger.DEBUG,"EULA.Status == 0");
            eulaText = Locale.getById("EULA.eula_text.topic") + "\n\n" + Locale.getById("EULA.eula_text.paragraph") + "\n\n" + Locale.getById("EULA.eula_text.resume") + "\n\n";
            Base.navigator.showDialog("EULA ",eulaText,true,[new DialogButtonItem("EULA.buttons.accept",this.setEulaAccepted,0.4,[Keyboard.ENTER]),new DialogButtonItem("EULA.buttons.cancel",this.setEulaDeclined,0.6,[Keyboard.ESCAPE])],800,500);
         }
      }
      
      private function setEulaAccepted() : void
      {
         Api.call(Api.SET_EULA_ACCEPTED,[{"eula":1}]);
         EULA.Status = 1;
      }
      
      private function setEulaDeclined() : void
      {
         Api.call(Api.QUIT_GAME);
      }
      
      protected function onEulaHandler(event:Event) : void
      {
         EULA.self.removeEventListener(EULA.STATUS_EVENT,this.onEulaHandler);
         this.EULADeals();
      }
      
      override protected function init(... args) : void
      {
         this.vBox = new VBox(this);
         this.vBox.alignment = VBox.LEFT;
         this.vBox.spacing = 1;
         this.vBox.x = 50;
         this.vBox.y = 80;
         this.toGame = new MenuButton(this.vBox);
         this.toGame.$ = "extendedGUI.RootWindow.startGameButton";
         this.toGame.height = 35;
         this.toGame.addEventListener(MouseEvent.CLICK,this.onToGameClick);
         this.charPanel = new CharPanel(this.vBox);
         this.charPanel.paddingLeft = -10;
         this.premium = new NewPremiumPanel(this.vBox);
         this.premium.paddingTop = 5;
         this.premium.paddingLeft = -10;
         this.settings = new MenuButton(this.vBox);
         this.settings.$ = "extendedGUI.RootWindow.settingsButton";
         this.settings.height = 35;
         this.settings.paddingTop = 5;
         this.settings.addEventListener(MouseEvent.CLICK,this.onSettingsClick);
         this.account = new MenuButton(this.vBox);
         this.account.$ = "extendedGUI.RootWindow.accountButton";
         this.account.height = 35;
         this.account.addEventListener(MouseEvent.CLICK,this.onAccountClick);
         this.account.enabled = true;
         this.support = new MenuButton(this.vBox);
         this.support.$ = "extendedGUI.RootWindow.supportButton";
         this.support.height = 35;
         this.support.addEventListener(MouseEvent.CLICK,this.onSupportClick);
         this.news = new MenuButton(this.vBox);
         this.news.$ = "extendedGUI.RootWindow.newsButton";
         this.news.height = 35;
         this.news.addEventListener(MouseEvent.CLICK,this.onNewsClickHandler);
         this.licenseBtn = new MenuButton(this.vBox);
         this.licenseBtn.$ = "extendedGUI.RootWindow.licenseButton";
         this.licenseBtn.height = 35;
         this.licenseBtn.addEventListener(MouseEvent.CLICK,this.onLicenseClickHandler);
         var quad2:Quad = new Quad(this.vBox);
         quad2.paddingLeft = -10;
         quad2.width = 400;
         quad2.height = 10;
         this.quit = new MenuButton(this.vBox);
         this.quit.$ = "extendedGUI.RootWindow.quitButton";
         this.quit.height = 35;
         this.quit.addEventListener(MouseEvent.CLICK,this.onQuitClick);
         setTimeout(this.updateToGameButton,100);
      }
      
      override protected function resize(... args) : void
      {
         Logger.LogToChannel(Logger.DEBUG,this,"resize");
      }
      
      protected function onCharacterChange(event:Event) : void
      {
         this.updateToGameButton();
      }
      
      protected function updateToGameButton() : void
      {
         this.toGame.enabled = this.startAllowed;
      }
      
      protected function get startAllowed() : Boolean
      {
         return Character.list.length > 0 && Character.current != null && Boolean(Character.current.isNotDeleted);
      }
      
      protected function onToGameClick(event:MouseEvent) : void
      {
         Logger.LogToChannel(Logger.DEBUG,"MainScreen.onToGameClick");
         this.playWithTutorial();
      }
      
      private function askTutorialIfNeed() : void
      {
         if(Character.EveryoneNotPassed)
         {
            this.playWithTutorial();
         }
         else if(Character.CurrentNotPassed)
         {
            Base.navigator.showDialog("extendedGUI.RootWindow.askTutorialHeader","extendedGUI.RootWindow.askTutorialQuestion",true,[new DialogButtonItem("extendedGUI.Dialogs.Yes",this.playWithTutorial,0.4,[Keyboard.ENTER]),new DialogButtonItem("extendedGUI.Dialogs.No",this.playWithoutTutorial,0.6,[Keyboard.ESCAPE])],500,200);
         }
         else
         {
            this.playWithoutTutorial();
         }
      }
      
      private function playWithoutTutorial() : void
      {
         Api.call(Api.GO_TO_GAME,[{"tutorial_passed":1}]);
      }
      
      private function playWithTutorial() : void
      {
         Api.call(Api.GO_TO_GAME,[{"tutorial_passed":0}]);
      }
      
      protected function onNewsClickHandler(event:Event) : void
      {
         Api.call(Api.SHOW_NEWS);
      }
      
      protected function onSettingsClick(event:MouseEvent) : void
      {
         this.dispatchEvent(new ScreenEvent(ScreenEvent.GO_SCREEN,MainMenuGUI.BASE_SETTINGS));
      }
      
      protected function onAccountClick(event:MouseEvent) : void
      {
         Api.call(Api.OPEN_URL,[{"url":"https://www.stalker.so/kabinet/"}]);
      }
      
      protected function onSupportClick(event:MouseEvent) : void
      {
         Api.call(Api.OPEN_URL,[{"url":Locale.current.supportUrl}]);
      }
      
      protected function onQuitClick(event:MouseEvent) : void
      {
         this.goBack();
      }
      
      override public function goBack() : void
      {
         Base.navigator.showDialog("extendedGUI.Dialogs.Warning","extendedGUI.RootWindow.quitMessage",true,[new DialogButtonItem("extendedGUI.Dialogs.Yes",this.doGoBack,0.4,[Keyboard.ENTER]),new DialogButtonItem("extendedGUI.Dialogs.No",null,0.6,[Keyboard.ESCAPE])],500,200);
      }
      
      protected function onLicenseClickHandler(event:Event) : void
      {
         var eulaText:* = Locale.getById("EULA.eula_text.topic") + "\n\n" + Locale.getById("EULA.eula_text.paragraph") + "\n\n" + Locale.getById("EULA.eula_text.resume") + "\n\n";
         Base.navigator.showDialog("EULA ",eulaText,true,[new DialogButtonItem("extendedGUI.NewsWindow.closeDialog",null,1,[Keyboard.ENTER,Keyboard.ESCAPE])],800,500);
      }
      
      protected function doGoBack() : void
      {
         Auth.self.doLogout();
      }
      
      override public function showUp() : void
      {
         super.showUp();
         this.addEventListener(KeyboardEvent.KEY_DOWN,this.onEnterDown);
      }
      
      override public function showDown() : void
      {
         super.showDown();
         this.removeEventListener(KeyboardEvent.KEY_DOWN,this.onEnterDown);
      }
      
      private function onEnterDown(e:KeyboardEvent) : *
      {
         if(!this.startAllowed)
         {
            return;
         }
         if(e.keyCode == Keyboard.ENTER || e.keyCode == Keyboard.NUMPAD_ENTER)
         {
            this.onToGameClick(null);
         }
      }
   }
}

