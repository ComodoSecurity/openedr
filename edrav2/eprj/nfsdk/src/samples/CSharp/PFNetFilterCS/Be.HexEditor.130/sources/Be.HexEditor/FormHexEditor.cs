using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Text;
using System.IO;
using System.Windows.Forms;
using Be.Windows.Forms;

namespace Be.HexEditor
{
    public partial class FormHexEditor : Form
    {
        FormFind _formFind = new FormFind();
        FormFindCancel _formFindCancel;
        FormGoTo _formGoto = new FormGoTo();
        byte[] _findBuffer = new byte[0];
        string _fileName;

        public FormHexEditor()
        {
            InitializeComponent();

            Init();
        }

        /// <summary>
        /// Initializes the hex editor´s main form
        /// </summary>
        void Init()
        {
            DisplayText();

            ManageAbility();
        }

        /// <summary>
        /// Updates the File size status label
        /// </summary>
        void UpdateFileSizeStatus()
        {
            if (this.hexBox.ByteProvider == null)
                this.fileSizeToolStripStatusLabel.Text = string.Empty;
            else
                this.fileSizeToolStripStatusLabel.Text = Util.GetDisplayBytes(this.hexBox.ByteProvider.Length);
        }

        /// <summary>
        /// Displays the file name in the Form´s text property
        /// </summary>
        /// <param name="fileName">the file name to display</param>
        void DisplayText()
        {
            if (_fileName != null && _fileName.Length > 0)
            {
                string textFormat = "{0}{1} - {2}";
                string sReadOnly = ((DynamicFileByteProvider)hexBox.ByteProvider).ReadOnly
                    ? strings.Readonly : "";
                string text = Path.GetFileName(_fileName);
                this.Text = string.Format(textFormat, text, sReadOnly, Program.SOFTWARENAME);
            }
            else
            {
                this.Text = Program.SOFTWARENAME;
            }
        }

        /// <summary>
        /// Manages enabling or disabling of menu items and toolstrip buttons.
        /// </summary>
        void ManageAbility()
        {
            if (hexBox.ByteProvider == null)
            {
                saveToolStripMenuItem.Enabled = saveToolStripButton.Enabled = false;

                findToolStripMenuItem.Enabled = false;
                findNextToolStripMenuItem.Enabled = false;
                goToToolStripMenuItem.Enabled = false;
            }
            else
            {
                saveToolStripMenuItem.Enabled = saveToolStripButton.Enabled = hexBox.ByteProvider.HasChanges();

                findToolStripMenuItem.Enabled = true;
                findNextToolStripMenuItem.Enabled = true;
                goToToolStripMenuItem.Enabled = true;
            }

            ManageAbilityForCopyAndPaste();
        }

        /// <summary>
        /// Manages enabling or disabling of menustrip items and toolstrip buttons for copy and paste
        /// </summary>
        void ManageAbilityForCopyAndPaste()
        {
            copyToolStripButton.Enabled = copyToolStripMenuItem.Enabled = hexBox.CanCopy();
            cutToolStripButton.Enabled = cutToolStripMenuItem.Enabled = hexBox.CanCut();
            pasteToolStripButton.Enabled = pasteToolStripMenuItem.Enabled = hexBox.CanPaste();
        }

        /// <summary>
        /// Shows the open file dialog.
        /// </summary>
        void OpenFile()
        {
            if (openFileDialog.ShowDialog() == DialogResult.OK)
            {
                OpenFile(openFileDialog.FileName);
            }
        }

        /// <summary>
        /// Opens a file.
        /// </summary>
        /// <param name="fileName">the file name of the file to open</param>
        public void OpenFile(string fileName)
        {
            if (!File.Exists(fileName))
            {
                Program.ShowMessage(strings.FileDoesNotExist);
                return;
            }

            if (hexBox.ByteProvider != null)
            {
                if (CloseFile() == DialogResult.Cancel)
                    return;
            }

            try
            {
                DynamicFileByteProvider dynamicFileByteProvider;
                try
                {
                    // try to open in write mode
                    dynamicFileByteProvider = new DynamicFileByteProvider(fileName);
                    dynamicFileByteProvider.Changed += new EventHandler(byteProvider_Changed);
                    dynamicFileByteProvider.LengthChanged += new EventHandler(byteProvider_LengthChanged);
                }
                catch (IOException) // write mode failed
                {
                    try
                    {
                        // try to open in read-only mode
                        dynamicFileByteProvider = new DynamicFileByteProvider(fileName, true);
                        if (Program.ShowQuestion(strings.OpenReadonly) == DialogResult.No)
                        {
                            dynamicFileByteProvider.Dispose();
                            return;
                        }
                    }
                    catch (IOException) // read-only also failed
                    {
                        // file cannot be opened
                        Program.ShowError(strings.OpenFailed);
                        return;
                    }
                }
                
                hexBox.ByteProvider = dynamicFileByteProvider;
                _fileName = fileName;

                DisplayText();

                UpdateFileSizeStatus();

                recentFileHandler.AddFile(fileName);
            }
            catch (Exception ex1)
            {
                Program.ShowError(ex1);
                return;
            }
            finally
            {
                ManageAbility();
            }
        }

        /// <summary>
        /// Saves the current file.
        /// </summary>
        void SaveFile()
        {
            if (hexBox.ByteProvider == null)
                return;

            try
            {
                DynamicFileByteProvider dynamicFileByteProvider = hexBox.ByteProvider as DynamicFileByteProvider;
                dynamicFileByteProvider.ApplyChanges();
            }
            catch (Exception ex1)
            {
                Program.ShowError(ex1);
            }
            finally
            {
                ManageAbility();
            }
        }

        /// <summary>
        /// Closes the current file
        /// </summary>
        /// <returns>OK, if the current file was closed.</returns>
        DialogResult CloseFile()
        {
            if (hexBox.ByteProvider == null)
                return DialogResult.OK;

            try
            {
                if (hexBox.ByteProvider != null && hexBox.ByteProvider.HasChanges())
                {
                    DialogResult res = MessageBox.Show(strings.SaveChangesQuestion,
                        Program.SOFTWARENAME,
                        MessageBoxButtons.YesNoCancel,
                        MessageBoxIcon.Warning);

                    if (res == DialogResult.Yes)
                    {
                        SaveFile();
                        CleanUp();
                    }
                    else if (res == DialogResult.No)
                    {
                        CleanUp();
                    }
                    else if (res == DialogResult.Cancel)
                    {
                        return res;
                    }

                    return res;
                }
                else
                {
                    CleanUp();
                    return DialogResult.OK;
                }
            }
            finally
            {
                ManageAbility();
            }
        }

        void CleanUp()
        {
            if (hexBox.ByteProvider != null)
            {
                IDisposable byteProvider = hexBox.ByteProvider as IDisposable;
                if (byteProvider != null)
                    byteProvider.Dispose();
                hexBox.ByteProvider = null;
            }
            _fileName = null;
            DisplayText();
        }

        /// <summary>
        /// Opens the Find dialog
        /// </summary>
        void Find()
        {
            if (_formFind.ShowDialog() == DialogResult.OK)
            {
                _findBuffer = _formFind.GetFindBytes();
                FindNext();
            }
        }

        /// <summary>
        /// Find next match
        /// </summary>
        void FindNext()
        {
            if (_findBuffer.Length == 0)
            {
                Find();
                return;
            }

            // show cancel dialog
            _formFindCancel = new FormFindCancel();
            _formFindCancel.SetHexBox(hexBox);
            _formFindCancel.Closed += new EventHandler(FormFindCancel_Closed);
            _formFindCancel.Show();

            // block activation of main form
            Activated += new EventHandler(FocusToFormFindCancel);

            // start find process
            long res = hexBox.Find(_findBuffer, hexBox.SelectionStart + hexBox.SelectionLength);

            _formFindCancel.Dispose();

            // unblock activation of main form
            Activated -= new EventHandler(FocusToFormFindCancel);

            if (res == -1) // -1 = no match
            {
                MessageBox.Show(strings.FindOperationEndOfFile, Program.SOFTWARENAME,
                    MessageBoxButtons.OK, MessageBoxIcon.Information);
            }
            else if (res == -2) // -2 = find was aborted
            {
                return;
            }
            else // something was found
            {
                if (!hexBox.Focused)
                    hexBox.Focus();
            }

            ManageAbility();
        }

        /// <summary>
        /// Aborts the current find process
        /// </summary>
        void FormFindCancel_Closed(object sender, EventArgs e)
        {
            hexBox.AbortFind();
        }

        /// <summary>
        /// Put focus back to the cancel form.
        /// </summary>
        void FocusToFormFindCancel(object sender, EventArgs e)
        {
            _formFindCancel.Focus();
        }

        /// <summary>
        /// Displays the goto byte dialog.
        /// </summary>
        void Goto()
        {
            _formGoto.SetMaxByteIndex(hexBox.ByteProvider.Length);
            _formGoto.SetDefaultValue(hexBox.SelectionStart);
            if (_formGoto.ShowDialog() == DialogResult.OK)
            {
                hexBox.SelectionStart = _formGoto.GetByteIndex();
                hexBox.SelectionLength = 1;
                hexBox.Focus();
            }
        }

        /// <summary>
        /// Enables drag&drop
        /// </summary>
        void hexBox_DragEnter(object sender, System.Windows.Forms.DragEventArgs e)
        {
            e.Effect = DragDropEffects.All;
        }

        /// <summary>
        /// Processes a file drop
        /// </summary>
        void hexBox_DragDrop(object sender, System.Windows.Forms.DragEventArgs e)
        {
            string[] formats = e.Data.GetFormats();
            object oFileNames = e.Data.GetData(DataFormats.FileDrop);
            string[] fileNames = (string[])oFileNames;
            if (fileNames.Length == 1)
            {
                OpenFile(fileNames[0]);
            }
        }

        void hexBox_SelectionLengthChanged(object sender, System.EventArgs e)
        {
            ManageAbilityForCopyAndPaste();
        }

        void hexBox_SelectionStartChanged(object sender, System.EventArgs e)
        {
            ManageAbilityForCopyAndPaste();
        }

        void Position_Changed(object sender, EventArgs e)
        {
            this.toolStripStatusLabel.Text = string.Format("Ln {0}    Col {1}",
                hexBox.CurrentLine, hexBox.CurrentPositionInLine);
        }

        void byteProvider_Changed(object sender, EventArgs e)
        {
            ManageAbility();
        }

        void byteProvider_LengthChanged(object sender, EventArgs e)
        {
            UpdateFileSizeStatus();
        }

        void openToolStripItem_Click(object sender, EventArgs e)
        {
            OpenFile();
        }

        void saveToolStripItem_Click(object sender, EventArgs e)
        {
            SaveFile();
        }

        void saveAsToolStripItem_Click(object sender, EventArgs e)
        {
        }

        void cutToolStripItem_Click(object sender, EventArgs e)
        {
            this.hexBox.Cut();
        }

        void copyToolStripItem_Click(object sender, EventArgs e)
        {
            this.hexBox.Copy();
        }

        void pasteToolStripItem_Click(object sender, EventArgs e)
        {
            this.hexBox.Paste();
        }

        void findToolStripItem_Click(object sender, EventArgs e)
        {
            this.Find();
        }

        void findNextToolStripItem_Click(object sender, EventArgs e)
        {
            this.FindNext();
        }

        void goToToolStripItem_Click(object sender, EventArgs e)
        {
            this.Goto();
        }

        void exitToolStripItem_Click(object sender, EventArgs e)
        {
            this.Close();
        }

        void aboutToolStripMenuItem_Click(object sender, EventArgs e)
        {
            new FormAbout().ShowDialog();
        }

        void recentFilesToolStripMenuItem_DropDownItemClicked(object sender, ToolStripItemClickedEventArgs e)
        {
            RecentFileHandler.FileMenuItem fmi = (RecentFileHandler.FileMenuItem)e.ClickedItem;
            this.OpenFile(fmi.Filename);
        }

        void optionsToolStripMenuItem_Click(object sender, EventArgs e)
        {
            new FormOptions().ShowDialog();
        }
    }
}