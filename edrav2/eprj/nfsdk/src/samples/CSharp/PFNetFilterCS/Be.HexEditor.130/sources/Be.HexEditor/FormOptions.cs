using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Text;
using System.Windows.Forms;
using System.Globalization;
using Be.HexEditor.Properties;

namespace Be.HexEditor
{
    public partial class FormOptions : Form
    {
        int recentFilesMax;

        public int RecentFilesMax
        {
            get { return recentFilesMax; }
            set 
            {
                if (recentFilesMax == value)
                    return;
                if (value < 0 || value > RecentFileHandler.MAXRECENTFILES)
                    return;

                recentFilesMax = value; 
            }
        }

        bool useSystemLanguage;

        public bool UseSystemLanguage
        {
            get { return useSystemLanguage; }
            set { useSystemLanguage = value; }
        }

        public FormOptions()
        {
            InitializeComponent();

            this.recentFilesMax = Settings.Default.RecentFilesMax;
            this.recentFilesMaxTextBox.DataBindings.Add("Text", this, "RecentFilesMax");
            this.useSystemLanguage = Settings.Default.UseSystemLanguage;
            this.useSystemLanguageCheckBox.DataBindings.Add("Checked", this, "UseSystemLanguage");

            if (string.IsNullOrEmpty(Settings.Default.SelectedLanguage))
                Settings.Default.SelectedLanguage = CultureInfo.CurrentCulture.TwoLetterISOLanguageName; 

            DataTable dt = new DataTable();
            dt.Columns.Add("Name", typeof(string));
            dt.Columns.Add("Value", typeof(string));
            dt.Rows.Add(strings.English, "en");
            dt.Rows.Add(strings.German, "de");
            dt.DefaultView.Sort = "Name";

            this.languageComboBox.DataSource = dt.DefaultView;
            this.languageComboBox.DisplayMember = "Name";
            this.languageComboBox.ValueMember = "Value";
            this.languageComboBox.SelectedValue = Settings.Default.SelectedLanguage;
            if (this.languageComboBox.SelectedIndex == -1)
                this.languageComboBox.SelectedIndex = 0;
        }

        void clearRecentFilesButton_Click(object sender, EventArgs e)
        {
            Program.FormHexEditor.recentFileHandler.Clear();
        }

        void okButton_Click(object sender, EventArgs e)
        {
            bool changed = false;
            if (recentFilesMax != Settings.Default.RecentFilesMax)
            {
                Settings.Default.RecentFilesMax = recentFilesMax;
                changed = true;
            }

            if (Settings.Default.UseSystemLanguage != this.useSystemLanguage ||
                Settings.Default.SelectedLanguage != (string)this.languageComboBox.SelectedValue)
            {
                Settings.Default.UseSystemLanguage = this.UseSystemLanguage;
                Settings.Default.SelectedLanguage = (string)this.languageComboBox.SelectedValue;

                Program.ShowMessage(strings.ProgramRestartSettings);

                changed = true;
            }

            if(changed)
                Settings.Default.Save();

            this.DialogResult = DialogResult.OK;
        }

        void useSystemLanguageCheckBox_CheckedChanged(object sender, EventArgs e)
        {
            this.languageComboBox.Enabled = this.selectLanguageLabel.Enabled = !useSystemLanguageCheckBox.Checked;
        }
    }
}
