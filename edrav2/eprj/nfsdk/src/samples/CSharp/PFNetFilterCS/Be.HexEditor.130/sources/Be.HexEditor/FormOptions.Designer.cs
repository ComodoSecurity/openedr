namespace Be.HexEditor
{
    partial class FormOptions
    {
        /// <summary>
        /// Required designer variable.
        /// </summary>
        private System.ComponentModel.IContainer components = null;

        /// <summary>
        /// Clean up any resources being used.
        /// </summary>
        /// <param name="disposing">true if managed resources should be disposed; otherwise, false.</param>
        protected override void Dispose(bool disposing)
        {
            if (disposing && (components != null))
            {
                components.Dispose();
            }
            base.Dispose(disposing);
        }

        #region Windows Form Designer generated code

        /// <summary>
        /// Required method for Designer support - do not modify
        /// the contents of this method with the code editor.
        /// </summary>
        private void InitializeComponent()
        {
            System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(FormOptions));
            this.tabControl = new System.Windows.Forms.TabControl();
            this.generalTabPage = new System.Windows.Forms.TabPage();
            this.languageSettingsGroupBox = new System.Windows.Forms.GroupBox();
            this.selectLanguageLabel = new System.Windows.Forms.Label();
            this.languageComboBox = new System.Windows.Forms.ComboBox();
            this.useSystemLanguageCheckBox = new System.Windows.Forms.CheckBox();
            this.recentFilesGroupBox = new System.Windows.Forms.GroupBox();
            this.clearRecentFilesButton = new System.Windows.Forms.Button();
            this.recentFilesMaxlabel = new System.Windows.Forms.Label();
            this.recentFilesMaxTextBox = new System.Windows.Forms.TextBox();
            this.okButton = new System.Windows.Forms.Button();
            this.cancelButton = new System.Windows.Forms.Button();
            this.tabControl.SuspendLayout();
            this.generalTabPage.SuspendLayout();
            this.languageSettingsGroupBox.SuspendLayout();
            this.recentFilesGroupBox.SuspendLayout();
            this.SuspendLayout();
            // 
            // tabControl
            // 
            this.tabControl.AccessibleDescription = null;
            this.tabControl.AccessibleName = null;
            resources.ApplyResources(this.tabControl, "tabControl");
            this.tabControl.BackgroundImage = null;
            this.tabControl.Controls.Add(this.generalTabPage);
            this.tabControl.Font = null;
            this.tabControl.Name = "tabControl";
            this.tabControl.SelectedIndex = 0;
            // 
            // generalTabPage
            // 
            this.generalTabPage.AccessibleDescription = null;
            this.generalTabPage.AccessibleName = null;
            resources.ApplyResources(this.generalTabPage, "generalTabPage");
            this.generalTabPage.BackgroundImage = null;
            this.generalTabPage.Controls.Add(this.languageSettingsGroupBox);
            this.generalTabPage.Controls.Add(this.recentFilesGroupBox);
            this.generalTabPage.Font = null;
            this.generalTabPage.Name = "generalTabPage";
            this.generalTabPage.UseVisualStyleBackColor = true;
            // 
            // languageSettingsGroupBox
            // 
            this.languageSettingsGroupBox.AccessibleDescription = null;
            this.languageSettingsGroupBox.AccessibleName = null;
            resources.ApplyResources(this.languageSettingsGroupBox, "languageSettingsGroupBox");
            this.languageSettingsGroupBox.BackgroundImage = null;
            this.languageSettingsGroupBox.Controls.Add(this.selectLanguageLabel);
            this.languageSettingsGroupBox.Controls.Add(this.languageComboBox);
            this.languageSettingsGroupBox.Controls.Add(this.useSystemLanguageCheckBox);
            this.languageSettingsGroupBox.Font = null;
            this.languageSettingsGroupBox.Name = "languageSettingsGroupBox";
            this.languageSettingsGroupBox.TabStop = false;
            // 
            // selectLanguageLabel
            // 
            this.selectLanguageLabel.AccessibleDescription = null;
            this.selectLanguageLabel.AccessibleName = null;
            resources.ApplyResources(this.selectLanguageLabel, "selectLanguageLabel");
            this.selectLanguageLabel.Font = null;
            this.selectLanguageLabel.Name = "selectLanguageLabel";
            // 
            // languageComboBox
            // 
            this.languageComboBox.AccessibleDescription = null;
            this.languageComboBox.AccessibleName = null;
            resources.ApplyResources(this.languageComboBox, "languageComboBox");
            this.languageComboBox.BackgroundImage = null;
            this.languageComboBox.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
            this.languageComboBox.Font = null;
            this.languageComboBox.FormattingEnabled = true;
            this.languageComboBox.Name = "languageComboBox";
            // 
            // useSystemLanguageCheckBox
            // 
            this.useSystemLanguageCheckBox.AccessibleDescription = null;
            this.useSystemLanguageCheckBox.AccessibleName = null;
            resources.ApplyResources(this.useSystemLanguageCheckBox, "useSystemLanguageCheckBox");
            this.useSystemLanguageCheckBox.BackgroundImage = null;
            this.useSystemLanguageCheckBox.Font = null;
            this.useSystemLanguageCheckBox.Name = "useSystemLanguageCheckBox";
            this.useSystemLanguageCheckBox.UseVisualStyleBackColor = true;
            this.useSystemLanguageCheckBox.CheckedChanged += new System.EventHandler(this.useSystemLanguageCheckBox_CheckedChanged);
            // 
            // recentFilesGroupBox
            // 
            this.recentFilesGroupBox.AccessibleDescription = null;
            this.recentFilesGroupBox.AccessibleName = null;
            resources.ApplyResources(this.recentFilesGroupBox, "recentFilesGroupBox");
            this.recentFilesGroupBox.BackgroundImage = null;
            this.recentFilesGroupBox.Controls.Add(this.clearRecentFilesButton);
            this.recentFilesGroupBox.Controls.Add(this.recentFilesMaxlabel);
            this.recentFilesGroupBox.Controls.Add(this.recentFilesMaxTextBox);
            this.recentFilesGroupBox.Font = null;
            this.recentFilesGroupBox.Name = "recentFilesGroupBox";
            this.recentFilesGroupBox.TabStop = false;
            // 
            // clearRecentFilesButton
            // 
            this.clearRecentFilesButton.AccessibleDescription = null;
            this.clearRecentFilesButton.AccessibleName = null;
            resources.ApplyResources(this.clearRecentFilesButton, "clearRecentFilesButton");
            this.clearRecentFilesButton.BackgroundImage = null;
            this.clearRecentFilesButton.Font = null;
            this.clearRecentFilesButton.Name = "clearRecentFilesButton";
            this.clearRecentFilesButton.UseVisualStyleBackColor = true;
            this.clearRecentFilesButton.Click += new System.EventHandler(this.clearRecentFilesButton_Click);
            // 
            // recentFilesMaxlabel
            // 
            this.recentFilesMaxlabel.AccessibleDescription = null;
            this.recentFilesMaxlabel.AccessibleName = null;
            resources.ApplyResources(this.recentFilesMaxlabel, "recentFilesMaxlabel");
            this.recentFilesMaxlabel.Font = null;
            this.recentFilesMaxlabel.Name = "recentFilesMaxlabel";
            // 
            // recentFilesMaxTextBox
            // 
            this.recentFilesMaxTextBox.AccessibleDescription = null;
            this.recentFilesMaxTextBox.AccessibleName = null;
            resources.ApplyResources(this.recentFilesMaxTextBox, "recentFilesMaxTextBox");
            this.recentFilesMaxTextBox.BackgroundImage = null;
            this.recentFilesMaxTextBox.Font = null;
            this.recentFilesMaxTextBox.Name = "recentFilesMaxTextBox";
            // 
            // okButton
            // 
            this.okButton.AccessibleDescription = null;
            this.okButton.AccessibleName = null;
            resources.ApplyResources(this.okButton, "okButton");
            this.okButton.BackgroundImage = null;
            this.okButton.Font = null;
            this.okButton.Name = "okButton";
            this.okButton.UseVisualStyleBackColor = true;
            this.okButton.Click += new System.EventHandler(this.okButton_Click);
            // 
            // cancelButton
            // 
            this.cancelButton.AccessibleDescription = null;
            this.cancelButton.AccessibleName = null;
            resources.ApplyResources(this.cancelButton, "cancelButton");
            this.cancelButton.BackgroundImage = null;
            this.cancelButton.DialogResult = System.Windows.Forms.DialogResult.Cancel;
            this.cancelButton.Font = null;
            this.cancelButton.Name = "cancelButton";
            this.cancelButton.UseVisualStyleBackColor = true;
            // 
            // FormOptions
            // 
            this.AccessibleDescription = null;
            this.AccessibleName = null;
            resources.ApplyResources(this, "$this");
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.BackgroundImage = null;
            this.CancelButton = this.cancelButton;
            this.Controls.Add(this.cancelButton);
            this.Controls.Add(this.okButton);
            this.Controls.Add(this.tabControl);
            this.Font = null;
            this.Icon = null;
            this.MaximizeBox = false;
            this.MinimizeBox = false;
            this.Name = "FormOptions";
            this.tabControl.ResumeLayout(false);
            this.generalTabPage.ResumeLayout(false);
            this.languageSettingsGroupBox.ResumeLayout(false);
            this.languageSettingsGroupBox.PerformLayout();
            this.recentFilesGroupBox.ResumeLayout(false);
            this.recentFilesGroupBox.PerformLayout();
            this.ResumeLayout(false);

        }

        #endregion

        private System.Windows.Forms.TabControl tabControl;
        private System.Windows.Forms.TabPage generalTabPage;
        private System.Windows.Forms.Button okButton;
        private System.Windows.Forms.Button cancelButton;
        private System.Windows.Forms.GroupBox recentFilesGroupBox;
        private System.Windows.Forms.Button clearRecentFilesButton;
        private System.Windows.Forms.Label recentFilesMaxlabel;
        private System.Windows.Forms.TextBox recentFilesMaxTextBox;
        private System.Windows.Forms.GroupBox languageSettingsGroupBox;
        private System.Windows.Forms.Label selectLanguageLabel;
        private System.Windows.Forms.ComboBox languageComboBox;
        private System.Windows.Forms.CheckBox useSystemLanguageCheckBox;
    }
}