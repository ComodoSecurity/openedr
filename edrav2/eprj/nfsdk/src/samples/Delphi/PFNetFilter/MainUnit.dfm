object Form1: TForm1
  Left = 242
  Top = 153
  Width = 934
  Height = 702
  Caption = 'PFNetFilter'
  Color = clBtnFace
  Font.Charset = DEFAULT_CHARSET
  Font.Color = clWindowText
  Font.Height = -11
  Font.Name = 'MS Sans Serif'
  Font.Style = []
  OldCreateOrder = False
  Position = poDefaultPosOnly
  OnCreate = FormCreate
  OnDestroy = FormDestroy
  PixelsPerInch = 96
  TextHeight = 13
  object Splitter4: TSplitter
    Left = 0
    Top = 185
    Width = 918
    Height = 6
    Cursor = crVSplit
    Align = alBottom
  end
  object Panel1: TPanel
    Left = 0
    Top = 0
    Width = 918
    Height = 37
    Align = alTop
    BevelOuter = bvNone
    TabOrder = 0
    object StartBtn: TButton
      Left = 16
      Top = 7
      Width = 75
      Height = 25
      Caption = 'Start'
      TabOrder = 0
      OnClick = StartBtnClick
    end
    object StopBtn: TButton
      Left = 97
      Top = 7
      Width = 75
      Height = 25
      Caption = 'Stop'
      Enabled = False
      TabOrder = 1
      OnClick = StopBtnClick
    end
    object FilterSSL: TCheckBox
      Left = 376
      Top = 5
      Width = 217
      Height = 17
      Caption = 'Filter SSL-protected connections'
      Checked = True
      State = cbChecked
      TabOrder = 2
      OnClick = FilterSSLClick
    end
    object FilterRaw: TCheckBox
      Left = 376
      Top = 21
      Width = 217
      Height = 17
      Caption = 'Filter unclassified buffers as raw packets'
      TabOrder = 3
      OnClick = FilterRawClick
    end
    object SaveToBin: TCheckBox
      Left = 600
      Top = 4
      Width = 201
      Height = 17
      Caption = 'Save classified objects to *.bin files'
      TabOrder = 4
    end
    object Firewall: TCheckBox
      Left = 184
      Top = 5
      Width = 177
      Height = 17
      Caption = 'Interactive firewall (only for TCP)'
      TabOrder = 5
      OnClick = FirewallClick
    end
  end
  object Panel5: TPanel
    Left = 0
    Top = 37
    Width = 918
    Height = 148
    Align = alClient
    BevelOuter = bvNone
    TabOrder = 1
    object Splitter7: TSplitter
      Left = 626
      Top = 0
      Width = 5
      Height = 148
      Cursor = crHSplit
      Align = alRight
    end
    object GroupBox4: TGroupBox
      Left = 0
      Top = 0
      Width = 626
      Height = 148
      Align = alClient
      Caption = 'Sessions'
      TabOrder = 0
      object Sessions: TValueListEditor
        Left = 2
        Top = 15
        Width = 622
        Height = 131
        Align = alClient
        DisplayOptions = [doColumnTitles, doKeyColFixed]
        Options = [goFixedVertLine, goFixedHorzLine, goVertLine, goHorzLine, goColSizing, goThumbTracking]
        TabOrder = 0
        TitleCaptions.Strings = (
          'Id'
          'Properties')
        ColWidths = (
          60
          532)
      end
    end
    object GroupBox5: TGroupBox
      Left = 631
      Top = 0
      Width = 287
      Height = 148
      Align = alRight
      Caption = 'Statistics'
      TabOrder = 1
      object Stat: TValueListEditor
        Left = 2
        Top = 15
        Width = 283
        Height = 131
        Align = alClient
        DisplayOptions = [doColumnTitles, doKeyColFixed]
        Options = [goFixedVertLine, goFixedHorzLine, goVertLine, goHorzLine, goColSizing, goThumbTracking]
        TabOrder = 0
        TitleCaptions.Strings = (
          '[Type] Process:Port'
          'Counter')
        ColWidths = (
          104
          173)
      end
    end
  end
  object Panel6: TPanel
    Left = 0
    Top = 191
    Width = 918
    Height = 454
    Align = alBottom
    BevelOuter = bvNone
    TabOrder = 2
    object Label5: TLabel
      Left = 0
      Top = 0
      Width = 918
      Height = 17
      Align = alTop
      AutoSize = False
      Caption = 'Classified content'
      Color = clBackground
      Font.Charset = DEFAULT_CHARSET
      Font.Color = clWhite
      Font.Height = -11
      Font.Name = 'MS Sans Serif'
      Font.Style = []
      ParentColor = False
      ParentFont = False
      Layout = tlCenter
    end
    object PageControl1: TPageControl
      Left = 0
      Top = 17
      Width = 918
      Height = 437
      ActivePage = TabSheet1
      Align = alClient
      TabIndex = 0
      TabOrder = 0
      object TabSheet1: TTabSheet
        Caption = 'HTTP'
        DesignSize = (
          910
          409)
        object Label2: TLabel
          Left = 0
          Top = 8
          Width = 87
          Height = 13
          Caption = 'Stop-word for Urls:'
        end
        object Label3: TLabel
          Left = 224
          Top = 8
          Width = 99
          Height = 13
          Caption = 'Stop-word for HTML:'
        end
        object Label4: TLabel
          Left = 28
          Top = 31
          Width = 57
          Height = 13
          Caption = 'Block page:'
        end
        object Panel2: TPanel
          Left = 0
          Top = 88
          Width = 919
          Height = 319
          Anchors = [akLeft, akTop, akRight, akBottom]
          BevelOuter = bvLowered
          TabOrder = 3
          object Splitter1: TSplitter
            Left = 1
            Top = 169
            Width = 917
            Height = 6
            Cursor = crVSplit
            Align = alTop
            Beveled = True
          end
          object HttpList: TListView
            Left = 1
            Top = 1
            Width = 917
            Height = 168
            Align = alTop
            Columns = <
              item
                Caption = 'SessionId'
                Width = 80
              end
              item
                Caption = 'Type'
                Width = 80
              end
              item
                Caption = 'Status'
                Width = 200
              end
              item
                Caption = 'URL'
                Width = 250
              end
              item
                Alignment = taRightJustify
                Caption = 'Content length'
                Width = 100
              end
              item
                Caption = 'Action'
                Width = 80
              end
              item
                Caption = 'Session info'
                Width = 250
              end>
            GridLines = True
            HideSelection = False
            ReadOnly = True
            RowSelect = True
            TabOrder = 0
            ViewStyle = vsReport
            OnSelectItem = HttpListSelectItem
          end
          object GroupBox1: TGroupBox
            Left = 1
            Top = 175
            Width = 917
            Height = 143
            Align = alClient
            Caption = 'Content'
            TabOrder = 1
            object Splitter2: TSplitter
              Left = 297
              Top = 15
              Width = 4
              Height = 126
              Cursor = crHSplit
            end
            object HttpHeader: TValueListEditor
              Left = 2
              Top = 15
              Width = 295
              Height = 126
              Align = alLeft
              Options = [goFixedVertLine, goFixedHorzLine, goVertLine, goHorzLine, goColSizing, goThumbTracking]
              TabOrder = 0
              TitleCaptions.Strings = (
                'Name'
                'Value')
              ColWidths = (
                116
                173)
            end
            object HttpContent: TRichEdit
              Left = 301
              Top = 15
              Width = 614
              Height = 126
              Align = alClient
              ReadOnly = True
              ScrollBars = ssBoth
              TabOrder = 1
            end
          end
        end
        object UrlStopWord: TEdit
          Left = 92
          Top = 3
          Width = 121
          Height = 21
          TabOrder = 0
          OnChange = UrlStopWordChange
        end
        object HtmlStopWord: TEdit
          Left = 326
          Top = 3
          Width = 121
          Height = 21
          TabOrder = 1
          OnChange = HtmlStopWordChange
        end
        object BlockHtml: TMemo
          Left = 92
          Top = 27
          Width = 804
          Height = 59
          Anchors = [akLeft, akTop, akRight]
          Lines.Strings = (
            
              '<html><body bgcolor=#f0f0f0><center><h1>Request blocked</h1></ce' +
              'nter></body></html>'
            '<!--'
            '   - Unfortunately, Microsoft has added a clever new'
            '   - "feature" to Internet Explorer. If the text of'
            '   - an error'#39's message is "too small", specifically'
            '   - less than 512 bytes, Internet Explorer returns'
            '   - its own error message. You can turn that off,'
            '   - but it'#39's pretty tricky to find switch called'
            '   - "smart error messages". That means, of course,'
            '   - that short error messages are censored by default.'
            '   - IIS always returns error messages that are long'
            '   - enough to make Internet Explorer happy. The'
            '   - workaround is pretty simple: pad the error'
            '   - message with a big comment like this to push it'
            '   - over the five hundred and twelve bytes minimum.'
            '   - Of course, that'#39's exactly what you'#39're reading'
            '   - right now.'
            '   -->')
          ScrollBars = ssVertical
          TabOrder = 2
          OnChange = BlockHtmlChange
        end
      end
      object TabSheet2: TTabSheet
        Caption = 'POP3/SMTP/NNTP'
        ImageIndex = 1
        DesignSize = (
          910
          409)
        object Label1: TLabel
          Left = 28
          Top = 8
          Width = 258
          Height = 13
          Caption = 'Block outgoing messages with specified address in To:'
        end
        object Label6: TLabel
          Left = 48
          Top = 32
          Width = 238
          Height = 13
          Caption = 'Add a prefix to the subjects of incoming messages:'
        end
        object Panel4: TPanel
          Left = 0
          Top = 56
          Width = 920
          Height = 354
          Anchors = [akLeft, akTop, akRight, akBottom]
          BevelOuter = bvLowered
          TabOrder = 0
          object Splitter5: TSplitter
            Left = 1
            Top = 145
            Width = 918
            Height = 6
            Cursor = crVSplit
            Align = alTop
            Beveled = True
          end
          object MailList: TListView
            Left = 1
            Top = 1
            Width = 918
            Height = 144
            Align = alTop
            Columns = <
              item
                Caption = 'SessionId'
                Width = 80
              end
              item
                Caption = 'Type'
                Width = 120
              end
              item
                Caption = 'Length'
                Width = 200
              end
              item
                Caption = 'Session info'
                Width = 500
              end>
            GridLines = True
            HideSelection = False
            ReadOnly = True
            RowSelect = True
            TabOrder = 0
            ViewStyle = vsReport
            OnSelectItem = MailListSelectItem
          end
          object GroupBox3: TGroupBox
            Left = 1
            Top = 151
            Width = 918
            Height = 202
            Align = alClient
            Caption = 'Content'
            TabOrder = 1
            object MailContent: TRichEdit
              Left = 2
              Top = 15
              Width = 914
              Height = 185
              Align = alClient
              ReadOnly = True
              ScrollBars = ssBoth
              TabOrder = 0
            end
          end
        end
        object SMTPBlackAddress: TEdit
          Left = 300
          Top = 5
          Width = 277
          Height = 21
          TabOrder = 1
          OnChange = SMTPBlackAddressChange
        end
        object POP3Prefix: TEdit
          Left = 300
          Top = 29
          Width = 277
          Height = 21
          TabOrder = 2
          Text = '[Filtered] '
          OnChange = POP3PrefixChange
        end
      end
      object TabSheet3: TTabSheet
        Caption = 'FTP'
        ImageIndex = 3
        DesignSize = (
          910
          409)
        object Panel7: TPanel
          Left = -2
          Top = 2
          Width = 920
          Height = 405
          Anchors = [akLeft, akTop, akRight, akBottom]
          BevelOuter = bvLowered
          TabOrder = 0
          object Splitter6: TSplitter
            Left = 1
            Top = 145
            Width = 918
            Height = 6
            Cursor = crVSplit
            Align = alTop
            Beveled = True
          end
          object FTPList: TListView
            Left = 1
            Top = 1
            Width = 918
            Height = 144
            Align = alTop
            Columns = <
              item
                Caption = 'SessionId'
                Width = 80
              end
              item
                Caption = 'Type'
                Width = 100
              end
              item
                Caption = 'Length'
                Width = 100
              end
              item
                Caption = 'Session info'
                Width = 600
              end>
            GridLines = True
            HideSelection = False
            ReadOnly = True
            RowSelect = True
            TabOrder = 0
            ViewStyle = vsReport
            OnSelectItem = FTPListSelectItem
          end
          object GroupBox6: TGroupBox
            Left = 1
            Top = 151
            Width = 918
            Height = 253
            Align = alClient
            Caption = 'Content'
            TabOrder = 1
            object FTPContent: TRichEdit
              Left = 2
              Top = 15
              Width = 914
              Height = 236
              Align = alClient
              ReadOnly = True
              ScrollBars = ssBoth
              TabOrder = 0
            end
          end
        end
      end
      object TabSheet5: TTabSheet
        Caption = 'ICQ'
        ImageIndex = 4
        DesignSize = (
          910
          409)
        object Label7: TLabel
          Left = 27
          Top = 32
          Width = 166
          Height = 13
          Caption = 'Block messages with this substring:'
        end
        object Label8: TLabel
          Left = 28
          Top = 8
          Width = 166
          Height = 13
          Caption = 'Block chats with the contact (UIN):'
        end
        object Panel8: TPanel
          Left = -2
          Top = 80
          Width = 920
          Height = 329
          Anchors = [akLeft, akTop, akRight, akBottom]
          BevelOuter = bvLowered
          TabOrder = 0
          object Splitter8: TSplitter
            Left = 1
            Top = 145
            Width = 918
            Height = 6
            Cursor = crVSplit
            Align = alTop
            Beveled = True
          end
          object ICQList: TListView
            Left = 1
            Top = 1
            Width = 918
            Height = 144
            Align = alTop
            Columns = <
              item
                Caption = 'SessionId'
                Width = 80
              end
              item
                Caption = 'Type'
                Width = 120
              end
              item
                Caption = 'Action'
                Width = 80
              end
              item
                Caption = 'Length'
                Width = 200
              end
              item
                Caption = 'Session info'
                Width = 500
              end>
            GridLines = True
            HideSelection = False
            ReadOnly = True
            RowSelect = True
            TabOrder = 0
            ViewStyle = vsReport
            OnSelectItem = ICQListSelectItem
          end
          object GroupBox7: TGroupBox
            Left = 1
            Top = 151
            Width = 918
            Height = 177
            Align = alClient
            Caption = 'Content'
            TabOrder = 1
            object Splitter9: TSplitter
              Left = 297
              Top = 15
              Width = 4
              Height = 160
              Cursor = crHSplit
            end
            object ICQHeader: TValueListEditor
              Left = 2
              Top = 15
              Width = 295
              Height = 160
              Align = alLeft
              Options = [goFixedVertLine, goFixedHorzLine, goVertLine, goHorzLine, goColSizing, goThumbTracking]
              TabOrder = 0
              TitleCaptions.Strings = (
                'Name'
                'Value')
              ColWidths = (
                116
                173)
            end
            object ICQContent: TRichEdit
              Left = 301
              Top = 15
              Width = 615
              Height = 160
              Align = alClient
              ReadOnly = True
              ScrollBars = ssBoth
              TabOrder = 1
            end
          end
        end
        object ICQBlockUIN: TEdit
          Left = 196
          Top = 5
          Width = 277
          Height = 21
          TabOrder = 1
          OnChange = ICQBlockUINChange
        end
        object ICQBlockString: TEdit
          Left = 196
          Top = 29
          Width = 277
          Height = 21
          TabOrder = 2
          OnChange = ICQBlockStringChange
        end
        object ICQBlockFileTransfers: TCheckBox
          Left = 27
          Top = 55
          Width = 174
          Height = 17
          Caption = 'Block file transfer requests'
          TabOrder = 3
          OnClick = ICQBlockFileTransfersClick
        end
      end
      object TabSheet6: TTabSheet
        Caption = 'XMPP'
        ImageIndex = 5
        object Panel9: TPanel
          Left = 0
          Top = 0
          Width = 910
          Height = 409
          Align = alClient
          BevelOuter = bvLowered
          TabOrder = 0
          object Splitter10: TSplitter
            Left = 1
            Top = 145
            Width = 908
            Height = 6
            Cursor = crVSplit
            Align = alTop
            Beveled = True
          end
          object XMPPList: TListView
            Left = 1
            Top = 1
            Width = 908
            Height = 144
            Align = alTop
            Columns = <
              item
                Caption = 'SessionId'
                Width = 80
              end
              item
                Caption = 'Type'
                Width = 100
              end
              item
                Caption = 'Length'
                Width = 100
              end
              item
                Caption = 'Session info'
                Width = 600
              end>
            GridLines = True
            HideSelection = False
            ReadOnly = True
            RowSelect = True
            TabOrder = 0
            ViewStyle = vsReport
            OnSelectItem = XMPPListSelectItem
          end
          object GroupBox8: TGroupBox
            Left = 1
            Top = 151
            Width = 908
            Height = 257
            Align = alClient
            Caption = 'Content'
            TabOrder = 1
            object XMPPContent: TRichEdit
              Left = 2
              Top = 15
              Width = 904
              Height = 240
              Align = alClient
              ReadOnly = True
              ScrollBars = ssBoth
              TabOrder = 0
            end
          end
        end
      end
      object TabSheet4: TTabSheet
        Caption = 'Raw Packets'
        ImageIndex = 3
        object Panel3: TPanel
          Left = 0
          Top = 0
          Width = 910
          Height = 409
          Align = alClient
          BevelOuter = bvLowered
          TabOrder = 0
          object Splitter3: TSplitter
            Left = 1
            Top = 145
            Width = 908
            Height = 6
            Cursor = crVSplit
            Align = alTop
            Beveled = True
          end
          object RawList: TListView
            Left = 1
            Top = 1
            Width = 908
            Height = 144
            Align = alTop
            Columns = <
              item
                Caption = 'SessionId'
                Width = 80
              end
              item
                Caption = 'Type'
                Width = 100
              end
              item
                Caption = 'Length'
                Width = 100
              end
              item
                Caption = 'Session info'
                Width = 600
              end>
            GridLines = True
            HideSelection = False
            ReadOnly = True
            RowSelect = True
            TabOrder = 0
            ViewStyle = vsReport
            OnSelectItem = RawListSelectItem
          end
          object GroupBox2: TGroupBox
            Left = 1
            Top = 151
            Width = 908
            Height = 257
            Align = alClient
            Caption = 'Content'
            TabOrder = 1
            object RawContent: TRichEdit
              Left = 2
              Top = 15
              Width = 904
              Height = 240
              Align = alClient
              ReadOnly = True
              ScrollBars = ssBoth
              TabOrder = 0
            end
          end
        end
      end
    end
  end
  object StatusBar1: TStatusBar
    Left = 0
    Top = 645
    Width = 918
    Height = 19
    Panels = <
      item
        Text = 'Configuration folder: c:\netfilter2'
        Width = 50
      end>
    SimplePanel = False
  end
end
