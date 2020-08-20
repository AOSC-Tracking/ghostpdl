package com.artifex.gsviewer.gui;

import java.awt.Adjustable;
import java.awt.Color;
import java.awt.Dimension;
import java.awt.Graphics;
import java.awt.Image;
import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;
import java.awt.event.AdjustmentEvent;
import java.awt.image.BufferedImage;
import java.util.ArrayList;
import java.util.List;
import javax.swing.Box;
import javax.swing.BoxLayout;
import javax.swing.ImageIcon;
import javax.swing.JButton;
import javax.swing.JComponent;
import javax.swing.JOptionPane;
import javax.swing.JPanel;
import javax.swing.JScrollPane;
import javax.swing.SwingUtilities;

import com.artifex.gsviewer.Document;
import com.artifex.gsviewer.Page;
import com.artifex.gsviewer.PageUpdateCallback;

/**
 * <p>Used to display documents into a window.</p>
 *
 * <p>Partially a auto-generated form using NetBeans.</p>
 */
public class ViewerWindow extends javax.swing.JFrame {

	private static final long serialVersionUID = 1L;

	/**
	 * The gap between each page in the main viewer, in pixels.
	 */
	public static final int VIEWER_PAGE_GAP = 10;

	/**
	 * The gap between each page in the mini viewer, in pixels.
	 */
	public static final int MINI_VIEWER_PAGE_GAP = 10;

	public static final int YES = JOptionPane.YES_OPTION;
	public static final int NO = JOptionPane.NO_OPTION;
	public static final int CANCEL = JOptionPane.CANCEL_OPTION;

	private ViewerGUIListener guiListener;
	private int currentPage, maxPage;
	private double currentZoom;

	private Document loadedDocument;
	private ScrollMap scrollMap;
	private List<PagePanel> viewerPagePanels;
	private List<PagePanel> miniViewerPagePanels;

	private Dimension min;
	private Dimension max;

	private boolean gotoPage;

	/**
	 * Creates new ViewerWindow.
	 */
	public ViewerWindow() {
		this(null);
	}

	/**
	 * Creates a new ViewerWindow with a GUIListener.
	 *
	 * @param listener A ViewerGUIListener.
	 */
	public ViewerWindow(final ViewerGUIListener listener) {
		initComponents();
		this.currentPage = 0;
		this.maxPage = 0;
		this.currentZoom = 1.0;
		this.viewerPagePanels = new ArrayList<>();
		this.miniViewerPagePanels = new ArrayList<>();

		// Adjustment listener looks for any change in the scroll value of the scrollbar
		this.viewerScrollPane.getVerticalScrollBar().addAdjustmentListener((AdjustmentEvent evt) -> {
			if (scrollMap != null) {
				Adjustable adjustable = evt.getAdjustable();
				int currentPage = scrollMap.getPageFor(adjustable.getValue());
				if (ViewerWindow.this.currentPage != currentPage) {
					if (guiListener != null)
						guiListener.onPageChange(ViewerWindow.this.currentPage, currentPage);
				}
				if (guiListener != null)
					guiListener.onScrollChange(adjustable.getValue());
				ViewerWindow.this.currentPage = currentPage;
				assumePage(currentPage);
			}
			refreshButtons();
		});

		setGUIListener(listener);

		zoomSlider.setEnabled(false);
		increaseZoomButton.setEnabled(false);
		decreaseZoomButton.setEnabled(false);
		zoomSlider.setValue(50);

		nextPageButton.setEnabled(false);
		lastPageButton.setEnabled(false);
		pageNumberField.setEditable(false);

		setLocationRelativeTo(null);

		min = getMinimumSize();
		max = getMaximumSize();

		gotoPage = false;
	}

	/**
	 * This method is called from within the constructor to initialize the form.
	 * WARNING: Do NOT modify this code. The content of this method is always
	 * regenerated by the Form Editor.
	 */

	// <editor-fold defaultstate="collapsed" desc="Generated
	// Code">//GEN-BEGIN:initComponents
	private void initComponents() {

        viewerScrollPane = new javax.swing.JScrollPane();
        viewerContentPane = new javax.swing.JPanel();
        miniViewerScrollPane = new javax.swing.JScrollPane();
        miniViewerContentPane = new javax.swing.JPanel();
        zoomSlider = new javax.swing.JSlider();
        increaseZoomButton = new javax.swing.JButton();
        decreaseZoomButton = new javax.swing.JButton();
        lastPageButton = new javax.swing.JButton();
        maxPagesLabel = new javax.swing.JTextField();
        nextPageButton = new javax.swing.JButton();
        progressBar = new javax.swing.JProgressBar();
        pageNumberField = new javax.swing.JTextField();
        pageSlashLabel = new javax.swing.JLabel();
        menuBar = new javax.swing.JMenuBar();
        fileMenu = new javax.swing.JMenu();
        openMenu = new javax.swing.JMenuItem();
        closeMenuItem = new javax.swing.JMenuItem();
        fileMenuSeparator = new javax.swing.JPopupMenu.Separator();
        exitMenuItem = new javax.swing.JMenuItem();
        editMenu = new javax.swing.JMenu();
        settingsMenuItem = new javax.swing.JMenuItem();

        setDefaultCloseOperation(javax.swing.WindowConstants.DO_NOTHING_ON_CLOSE);
        setTitle("Viewer");
        setMinimumSize(new java.awt.Dimension(640, 650));
        addWindowListener(new java.awt.event.WindowAdapter() {
            @Override
			public void windowClosing(java.awt.event.WindowEvent evt) {
                formWindowClosing(evt);
            }
        });

        viewerScrollPane.setMinimumSize(new java.awt.Dimension(85, 110));

        javax.swing.GroupLayout viewerContentPaneLayout = new javax.swing.GroupLayout(viewerContentPane);
        viewerContentPane.setLayout(viewerContentPaneLayout);
        viewerContentPaneLayout.setHorizontalGroup(
            viewerContentPaneLayout.createParallelGroup(javax.swing.GroupLayout.Alignment.LEADING)
            .addGap(0, 502, Short.MAX_VALUE)
        );
        viewerContentPaneLayout.setVerticalGroup(
            viewerContentPaneLayout.createParallelGroup(javax.swing.GroupLayout.Alignment.LEADING)
            .addGap(0, 639, Short.MAX_VALUE)
        );

        viewerScrollPane.setViewportView(viewerContentPane);

        miniViewerScrollPane.setHorizontalScrollBarPolicy(javax.swing.ScrollPaneConstants.HORIZONTAL_SCROLLBAR_NEVER);

        javax.swing.GroupLayout miniViewerContentPaneLayout = new javax.swing.GroupLayout(miniViewerContentPane);
        miniViewerContentPane.setLayout(miniViewerContentPaneLayout);
        miniViewerContentPaneLayout.setHorizontalGroup(
            miniViewerContentPaneLayout.createParallelGroup(javax.swing.GroupLayout.Alignment.LEADING)
            .addGap(0, 118, Short.MAX_VALUE)
        );
        miniViewerContentPaneLayout.setVerticalGroup(
            miniViewerContentPaneLayout.createParallelGroup(javax.swing.GroupLayout.Alignment.LEADING)
            .addGap(0, 639, Short.MAX_VALUE)
        );

        miniViewerScrollPane.setViewportView(miniViewerContentPane);

        zoomSlider.setMajorTickSpacing(25);
        zoomSlider.setMinorTickSpacing(5);
        zoomSlider.setPaintTicks(true);
        zoomSlider.setSnapToTicks(true);
        zoomSlider.addMouseListener(new java.awt.event.MouseAdapter() {
            @Override
			public void mouseReleased(java.awt.event.MouseEvent evt) {
                zoomSliderMouseReleased(evt);
            }
        });
        zoomSlider.addKeyListener(new java.awt.event.KeyAdapter() {
        @Override
                public void keyReleased(java.awt.event.KeyEvent evt) {
                        zoomSliderKeyReleased(evt);
                }
        });

        increaseZoomButton.setText("+");
        increaseZoomButton.addActionListener(new java.awt.event.ActionListener() {
            @Override
			public void actionPerformed(java.awt.event.ActionEvent evt) {
                increaseZoomButtonActionPerformed(evt);
            }
        });

        decreaseZoomButton.setText("-");
        decreaseZoomButton.addActionListener(new java.awt.event.ActionListener() {
            @Override
			public void actionPerformed(java.awt.event.ActionEvent evt) {
                decreaseZoomButtonActionPerformed(evt);
            }
        });

        lastPageButton.setText("<");
        lastPageButton.addActionListener(new java.awt.event.ActionListener() {
            @Override
			public void actionPerformed(java.awt.event.ActionEvent evt) {
                lastPageButtonActionPerformed(evt);
            }
        });

        maxPagesLabel.setEditable(false);
        maxPagesLabel.setText("0");

        nextPageButton.setText(">");
        nextPageButton.addActionListener(new java.awt.event.ActionListener() {
            @Override
			public void actionPerformed(java.awt.event.ActionEvent evt) {
                nextPageButtonActionPerformed(evt);
            }
        });

        pageNumberField.setText("0");
        pageNumberField.addKeyListener(new java.awt.event.KeyAdapter() {
            @Override
			public void keyPressed(java.awt.event.KeyEvent evt) {
                pageNumberFieldKeyPressed(evt);
            }
        });

        pageSlashLabel.setText("/");

        fileMenu.setText("File");

        openMenu.setText("Open");
        openMenu.addActionListener(new java.awt.event.ActionListener() {
            @Override
			public void actionPerformed(java.awt.event.ActionEvent evt) {
                openMenuActionPerformed(evt);
            }
        });
        fileMenu.add(openMenu);

        closeMenuItem.setText("Close");
        closeMenuItem.addActionListener(new java.awt.event.ActionListener() {
            @Override
			public void actionPerformed(java.awt.event.ActionEvent evt) {
                closeMenuItemActionPerformed(evt);
            }
        });
        fileMenu.add(closeMenuItem);
        fileMenu.add(fileMenuSeparator);

        exitMenuItem.setText("Exit");
        exitMenuItem.addActionListener(new java.awt.event.ActionListener() {
            @Override
			public void actionPerformed(java.awt.event.ActionEvent evt) {
                exitMenuItemActionPerformed(evt);
            }
        });
        fileMenu.add(exitMenuItem);

        menuBar.add(fileMenu);

        editMenu.setText("Edit");

        settingsMenuItem.setText("Settings");
        settingsMenuItem.addActionListener(new java.awt.event.ActionListener() {
            @Override
			public void actionPerformed(java.awt.event.ActionEvent evt) {
                settingsMenuItemActionPerformed(evt);
            }
        });
        editMenu.add(settingsMenuItem);

        menuBar.add(editMenu);

        setJMenuBar(menuBar);

        javax.swing.GroupLayout layout = new javax.swing.GroupLayout(getContentPane());
        getContentPane().setLayout(layout);
        layout.setHorizontalGroup(
            layout.createParallelGroup(javax.swing.GroupLayout.Alignment.LEADING)
            .addGroup(layout.createSequentialGroup()
                .addContainerGap()
                .addGroup(layout.createParallelGroup(javax.swing.GroupLayout.Alignment.LEADING)
                    .addComponent(progressBar, javax.swing.GroupLayout.DEFAULT_SIZE, javax.swing.GroupLayout.DEFAULT_SIZE, Short.MAX_VALUE)
                    .addGroup(layout.createSequentialGroup()
                        .addComponent(miniViewerScrollPane, javax.swing.GroupLayout.PREFERRED_SIZE, 120, javax.swing.GroupLayout.PREFERRED_SIZE)
                        .addPreferredGap(javax.swing.LayoutStyle.ComponentPlacement.UNRELATED)
                        .addGroup(layout.createParallelGroup(javax.swing.GroupLayout.Alignment.LEADING)
                            .addGroup(layout.createSequentialGroup()
                                .addGap(2, 2, 2)
                                .addComponent(decreaseZoomButton, javax.swing.GroupLayout.PREFERRED_SIZE, 41, javax.swing.GroupLayout.PREFERRED_SIZE)
                                .addPreferredGap(javax.swing.LayoutStyle.ComponentPlacement.RELATED)
                                .addComponent(zoomSlider, javax.swing.GroupLayout.PREFERRED_SIZE, 111, javax.swing.GroupLayout.PREFERRED_SIZE)
                                .addPreferredGap(javax.swing.LayoutStyle.ComponentPlacement.RELATED)
                                .addComponent(increaseZoomButton)
                                .addPreferredGap(javax.swing.LayoutStyle.ComponentPlacement.RELATED, javax.swing.GroupLayout.DEFAULT_SIZE, Short.MAX_VALUE)
                                .addComponent(lastPageButton)
                                .addPreferredGap(javax.swing.LayoutStyle.ComponentPlacement.RELATED)
                                .addComponent(pageNumberField, javax.swing.GroupLayout.PREFERRED_SIZE, 45, javax.swing.GroupLayout.PREFERRED_SIZE)
                                .addPreferredGap(javax.swing.LayoutStyle.ComponentPlacement.RELATED)
                                .addComponent(pageSlashLabel)
                                .addPreferredGap(javax.swing.LayoutStyle.ComponentPlacement.RELATED)
                                .addComponent(maxPagesLabel, javax.swing.GroupLayout.PREFERRED_SIZE, 47, javax.swing.GroupLayout.PREFERRED_SIZE)
                                .addGap(4, 4, 4)
                                .addComponent(nextPageButton)
                                .addGap(2, 2, 2))
                            .addComponent(viewerScrollPane, javax.swing.GroupLayout.DEFAULT_SIZE, javax.swing.GroupLayout.DEFAULT_SIZE, Short.MAX_VALUE))))
                .addContainerGap())
        );
        layout.setVerticalGroup(
            layout.createParallelGroup(javax.swing.GroupLayout.Alignment.LEADING)
            .addGroup(layout.createSequentialGroup()
                .addContainerGap()
                .addGroup(layout.createParallelGroup(javax.swing.GroupLayout.Alignment.LEADING)
                    .addComponent(miniViewerScrollPane)
                    .addComponent(viewerScrollPane, javax.swing.GroupLayout.DEFAULT_SIZE, javax.swing.GroupLayout.DEFAULT_SIZE, Short.MAX_VALUE))
                .addPreferredGap(javax.swing.LayoutStyle.ComponentPlacement.RELATED)
                .addGroup(layout.createParallelGroup(javax.swing.GroupLayout.Alignment.TRAILING)
                    .addGroup(layout.createParallelGroup(javax.swing.GroupLayout.Alignment.LEADING)
                        .addComponent(decreaseZoomButton)
                        .addGroup(layout.createParallelGroup(javax.swing.GroupLayout.Alignment.BASELINE)
                            .addComponent(increaseZoomButton)
                            .addComponent(nextPageButton)
                            .addComponent(maxPagesLabel, javax.swing.GroupLayout.PREFERRED_SIZE, javax.swing.GroupLayout.DEFAULT_SIZE, javax.swing.GroupLayout.PREFERRED_SIZE)
                            .addComponent(pageNumberField, javax.swing.GroupLayout.PREFERRED_SIZE, javax.swing.GroupLayout.DEFAULT_SIZE, javax.swing.GroupLayout.PREFERRED_SIZE)
                            .addComponent(lastPageButton)
                            .addComponent(pageSlashLabel)))
                    .addComponent(zoomSlider, javax.swing.GroupLayout.PREFERRED_SIZE, javax.swing.GroupLayout.DEFAULT_SIZE, javax.swing.GroupLayout.PREFERRED_SIZE))
                .addGap(14, 14, 14)
                .addComponent(progressBar, javax.swing.GroupLayout.DEFAULT_SIZE, javax.swing.GroupLayout.DEFAULT_SIZE, javax.swing.GroupLayout.PREFERRED_SIZE)
                .addContainerGap())
        );

        pack();
    }// </editor-fold>

	private void lastPageButtonActionPerformed(java.awt.event.ActionEvent evt) {// GEN-FIRST:event_lastPageButtonActionPerformed
		if (maxPage > 0) {
			tryChangePage(currentPage - 1);
		} else {
			pageNumberField.setText(new StringBuilder().append(0).toString());
		}
	}// GEN-LAST:event_lastPageButtonActionPerformed

	private void nextPageButtonActionPerformed(java.awt.event.ActionEvent evt) {// GEN-FIRST:event_nextPageButtonActionPerformed
		if (maxPage > 0) {
			tryChangePage(currentPage + 1);
		} else {
			pageNumberField.setText(new StringBuilder().append(0).toString());
		}
	}// GEN-LAST:event_nextPageButtonActionPerformed

	private void formWindowClosing(java.awt.event.WindowEvent evt) {// GEN-FIRST:event_formWindowClosing
		if (guiListener != null)
			guiListener.onClosing();
	}// GEN-LAST:event_formWindowClosing

	private void pageNumberFieldKeyPressed(java.awt.event.KeyEvent evt) {// GEN-FIRST:event_pageNumberFieldKeyPressed
		if (evt.getKeyCode() == java.awt.event.KeyEvent.VK_ENTER) {
			try {
				final String text = pageNumberField.getText();
				tryChangePage(Integer.parseInt(text));
			} catch (NumberFormatException e) {
				System.err.println("Invalid page numer.");
			}
		}
	}// GEN-LAST:event_pageNumberFieldKeyPressed

	private void zoomSliderMouseReleased(java.awt.event.MouseEvent evt) {// GEN-FIRST:event_zoomSliderMouseReleased
		tryChangeZoom(zoomSlider.getValue() / 50.0);
	}// GEN-LAST:event_zoomSliderMouseReleased

	private void zoomSliderKeyReleased(java.awt.event.KeyEvent evt) {// GEN-FIRST:event_zoomSliderKeyReleased
		tryChangeZoom(zoomSlider.getValue() / 50.0);
	}// GEN-LAST:event_zoomSliderKeyReleased

	private void increaseZoomButtonActionPerformed(java.awt.event.ActionEvent evt) {// GEN-FIRST:event_increaseZoomButtonActionPerformed
		if (tryChangeZoom(Math.min(Math.ceil((currentZoom * 10) + 1) / 10, 2.0)))
			zoomSlider.setValue((int)(currentZoom * 50));
	}// GEN-LAST:event_increaseZoomButtonActionPerformed

	private void decreaseZoomButtonActionPerformed(java.awt.event.ActionEvent evt) {// GEN-FIRST:event_decreaseZoomButtonActionPerformed
		if (tryChangeZoom(Math.max(Math.floor((currentZoom * 10) - 1) / 10, 0.0)))
			zoomSlider.setValue((int)(currentZoom * 50));
	}// GEN-LAST:event_decreaseZoomButtonActionPerformed

	private void openMenuActionPerformed(java.awt.event.ActionEvent evt) {// GEN-FIRST:event_openMenuActionPerformed
		if (guiListener != null)
			guiListener.onOpenFile();
	}// GEN-LAST:event_openMenuActionPerformed

	private void closeMenuItemActionPerformed(java.awt.event.ActionEvent evt) {// GEN-FIRST:event_closeMenuItemActionPerformed
		if (guiListener != null)
			guiListener.onCloseFile();
	}// GEN-LAST:event_closeMenuItemActionPerformed

	private void exitMenuItemActionPerformed(java.awt.event.ActionEvent evt) {// GEN-FIRST:event_exitMenuItemActionPerformed
		if (guiListener != null)
			guiListener.onClosing();
	}// GEN-LAST:event_exitMenuItemActionPerformed

	private void settingsMenuItemActionPerformed(java.awt.event.ActionEvent evt) {// GEN-FIRST:event_settingsMenuItemActionPerformed
		if (guiListener != null)
			guiListener.onSettingsOpen();
	}// GEN-LAST:event_settingsMenuItemActionPerformed

	// Variables declaration - do not modify//GEN-BEGIN:variables
	private javax.swing.JMenuItem closeMenuItem;
	private javax.swing.JButton decreaseZoomButton;
	private javax.swing.JMenu editMenu;
	private javax.swing.JMenuItem exitMenuItem;
	private javax.swing.JMenu fileMenu;
	private javax.swing.JPopupMenu.Separator fileMenuSeparator;
	private javax.swing.JButton increaseZoomButton;
	private javax.swing.JButton lastPageButton;
	private javax.swing.JTextField maxPagesLabel;
	private javax.swing.JMenuBar menuBar;
	private javax.swing.JPanel miniViewerContentPane;
	private javax.swing.JScrollPane miniViewerScrollPane;
	private javax.swing.JButton nextPageButton;
	private javax.swing.JMenuItem openMenu;
	private javax.swing.JTextField pageNumberField;
	private javax.swing.JLabel pageSlashLabel;
	private javax.swing.JProgressBar progressBar;
	private javax.swing.JMenuItem settingsMenuItem;
	private javax.swing.JPanel viewerContentPane;
	private javax.swing.JScrollPane viewerScrollPane;
	private javax.swing.JSlider zoomSlider;
	// End of variables declaration//GEN-END:variables

	private class PagePanel extends JPanel implements PageUpdateCallback {

		private static final long serialVersionUID = 1L;

		private Object lock;
		private Page page;
		private double size;

		private Image toDraw;

		private PagePanel(final Page page, double size) {
			this.lock = new Object();
			this.page = page;
			this.size = size == 0.0 ? 0.01 : size;

			Dimension pageSize = page.getSize();

			Dimension actualSize = new Dimension((int)(pageSize.width * size),
					(int)(pageSize.height * size));

			toDraw = getImage();

			setPreferredSize(actualSize);
			setMaximumSize(actualSize);

			setBackground(Color.WHITE);

			setDoubleBuffered(false);
			//pack();

			page.addCallback(this);
		}

		public Image getImage() {
			if (page != null && page.getLowResImage() != null) {
				Dimension pageSize = page.getSize();
				Dimension actualSize = new Dimension((int)(pageSize.width * size),
						(int)(pageSize.height * size));
				BufferedImage img = page.getDisplayableImage();
				if (page.getZoomedImage() != null && size > 1.0)
					img = page.getZoomedImage();

				Image result = img;
				if (img == page.getLowResImage() || currentZoom < 1.0 ||
						(img == page.getHighResImage() && currentZoom > 1.0 && page.getZoomedImage() == null)) {
					result = img.getScaledInstance(actualSize.width, actualSize.height, Image.SCALE_FAST);
				}
				return result;
			} else {
				return null;
			}
		}

		private void cleanup() {
			if (page != null) {
				page.removeCallback(this);
				page = null;
			}
		}

		@Override
		public void paintComponent(Graphics g) {
			super.paintComponent(g);
			synchronized (lock) {
				if (toDraw != null) {
					g.drawImage(toDraw, 0, 0, this);
				}
			}
		}

		@Override
		public void onPageUpdate() {
			toDraw = getImage();
			SwingUtilities.invokeLater(() -> {
				repaint();
			});
		}

		@Override
		public void onLoadLowRes() { }

		@Override
		public void onUnloadLowRes() { }

		@Override
		public void onLoadHighRes() { }

		@Override
		public void onUnloadHighRes() { }

		@Override
		public void onLoadZoomed() { }

		@Override
		public void onUnloadZoomed() { }
	}

	private class MiniViewerActionListener implements ActionListener {

		private final int pageNum;

		private MiniViewerActionListener(int pageNum) {
			this.pageNum = pageNum;
		}

		@Override
		public void actionPerformed(ActionEvent e) {
			tryChangePage(pageNum);
		}

	}

	@Override
	public void validate() {
		super.validate();
		if (this.scrollMap != null) {
			assumePage(this.currentPage = this.scrollMap.getCurrentPage());
			if (gotoPage)
				scrollMap.scrollTo(currentPage);
		}
		unlockSize();
		gotoPage = false;
	}

	/**
	 * Sets the ViewerGUIListener the window should use.
	 *
	 * @param listener A listener.
	 */
	public void setGUIListener(final ViewerGUIListener listener) {
		this.guiListener = listener;
		listener.onViewerAdd(this);
	}

	/**
	 * Sets the progress of the progress bar in the window.
	 *
	 * @param progress The amount of progress (0-100).
	 */
	public void setLoadProgress(final int progress) {
		progressBar.setValue(progress);
	}

	/**
	 * Loads a document into the viewer.
	 *
	 * @param document The document to load, if <code>null</code> the current
	 * document will be unloaded.
	 */
	public void loadDocumentToViewer(final Document document) {
		unloadViewerDocument();
		if (document == null)
			return;

		lockSize();

		this.loadedDocument = document;
		viewerContentPane.setLayout(new BoxLayout(viewerContentPane, BoxLayout.Y_AXIS));

		// Generate the viewer page components
		for (final Page page : document) {
			final PagePanel panel = new PagePanel(page, 1.0);
			viewerContentPane.add(panel);
			viewerContentPane.add(Box.createVerticalStrut(VIEWER_PAGE_GAP));

			viewerPagePanels.add(panel);
		}

		miniViewerContentPane.setLayout(new BoxLayout(miniViewerContentPane, BoxLayout.Y_AXIS));

		// Generate the mini viewer icons
		int pageNum = 1;
		for (final Page page : document) {
			ImageIcon icon = new ImageIcon(page.getLowResImage());
			JButton button = new JButton(icon);
			button.setPreferredSize(page.getLowResSize());
			button.addActionListener(new MiniViewerActionListener(pageNum++));
			button.setFocusable(false);

			miniViewerContentPane.add(button);
			miniViewerContentPane.add(Box.createVerticalStrut(MINI_VIEWER_PAGE_GAP));
		}

		this.scrollMap = new ScrollMap(document, this, VIEWER_PAGE_GAP);
		this.scrollMap.getScroll(1);

		assumePage(this.currentPage = 1);
		assumeMaxPages(this.maxPage = document.size());

		setTitle("Viewer - " + document.getName());

		zoomSlider.setEnabled(true);
		increaseZoomButton.setEnabled(true);
		decreaseZoomButton.setEnabled(true);
		zoomSlider.setValue(50);
		zoomSlider.setFocusable(true);

		nextPageButton.setEnabled(true);
		lastPageButton.setEnabled(true);
		pageNumberField.setEditable(true);

		refreshButtons();
	}

	public void unloadViewerDocument() {
		lockSize();

		this.loadedDocument = null;
		this.scrollMap = null;
		this.assumePage(0);
		this.assumeMaxPages(0);
		this.zoomSlider.setValue(50);
		this.zoomSlider.setEnabled(false);
		this.zoomSlider.setFocusable(false);
		this.pageNumberField.setEditable(false);
		setTitle("Viewer");

		for (final PagePanel panel : viewerPagePanels) {
			synchronized (panel.lock) {
				panel.cleanup();
			}
		}
		viewerContentPane.removeAll();
		viewerPagePanels.clear();

		miniViewerContentPane.removeAll();
		miniViewerPagePanels.clear();

		System.gc();

		SwingUtilities.invokeLater(() -> {
			viewerContentPane.revalidate();
			viewerContentPane.repaint();

			miniViewerContentPane.revalidate();
			miniViewerContentPane.repaint();
		});
	}

	/**
	 * Returns the document currently loaded in the viewer.
	 *
	 * @return The currently loaded document, or <code>null</code> if none
	 * is loaded.
	 */
	public Document getLoadedDocument() {
		return loadedDocument;
	}

	/**
	 * Returns the respective component inside of the viewer for a page.
	 *
	 * @param pageNum The page to get the component for.
	 * @return The respective component.
	 * @throws IndexOutOfBoundsException If <code>pageNum</code> is not a page in the document.
	 */
	public JComponent getPageComponent(final int pageNum) throws IndexOutOfBoundsException {
		return viewerPagePanels.get(pageNum - 1);
	}

	/**
	 * Returns the scroll pane used by the viewer.
	 *
	 * @return The scroll pane.
	 */
	public JScrollPane getViewerScrollPane() {
		return viewerScrollPane;
	}

	/**
	 * Tries to change the page to another page.
	 *
	 * @param newPage The page to change to.
	 * @return <code>true</code> if the page is a valid page and <code>false</code>
	 * otherwise.
	 */
	public boolean tryChangePage(final int newPage) {
		if (newPage < 1 || newPage > maxPage)
			return false;
		if (newPage != currentPage) {
			final int oldPage = currentPage;
			this.currentPage = newPage;
			if (guiListener != null)
				guiListener.onPageChange(oldPage, currentPage);
			if (this.scrollMap != null) {
				scrollMap.scrollTo(newPage);
				assumePage(newPage);
			}

			refreshButtons();

			pageNumberField.setText(new StringBuilder().append(currentPage).toString());
		}
		return true;
	}

	/**
	 * Tries to change the zoom to another zoom value.
	 *
	 * @param newZoom The zoom to change to.
	 * @return <code>true</code> if the zoom is a valid zoom value and
	 * <code>false</code> otherwise.
	 */
	public boolean tryChangeZoom(final double newZoom) {
		if (newZoom < 0 || newZoom > 2)
			return false;
		if (newZoom != currentZoom) {
			gotoPage = true;
			final double oldZoom = currentZoom;
			this.currentZoom = newZoom;
			if (guiListener != null)
				guiListener.onZoomChange(oldZoom, currentZoom);
			if (this.scrollMap != null) {
				redisplayDocument();
			}

			refreshButtons();
		}
		return true;
	}

	public int getCurrentPage() {
		if (this.scrollMap != null)
			return scrollMap.getCurrentPage();
		return 0;
	}

	public void showErrorDialog(String title, String message) {
		JOptionPane.showMessageDialog(this, message, title, JOptionPane.ERROR_MESSAGE);
	}

	public void showWarningDialog(String title, String message) {
		JOptionPane.showMessageDialog(this, message, title, JOptionPane.WARNING_MESSAGE);
	}

	public int showConfirmDialog(String title, String message) {
		return JOptionPane.showConfirmDialog(this, message, title, JOptionPane.YES_NO_CANCEL_OPTION);
	}

	public double getZoom() {
		return currentZoom;
	}

	/**
	 * Refresh each button's state.
	 */
	private void refreshButtons() {
		this.lastPageButton.setEnabled(currentPage != 1 && loadedDocument != null);
		this.nextPageButton.setEnabled(currentPage != maxPage && loadedDocument != null);

		this.increaseZoomButton.setEnabled(currentZoom != 2.0 && loadedDocument != null);
		this.decreaseZoomButton.setEnabled(currentZoom != 0.0 && loadedDocument != null);
	}

	private void redisplayDocument() {
		lockSize();

		for (final PagePanel panel : viewerPagePanels) {
			synchronized (panel.lock) {
				panel.cleanup();
			}
		}
		viewerContentPane.removeAll();
		viewerPagePanels.clear();

		System.gc();

		for (final Page page : loadedDocument) {
			final PagePanel panel = new PagePanel(page, currentZoom);
			viewerContentPane.add(panel);
			viewerContentPane.add(Box.createVerticalStrut(VIEWER_PAGE_GAP));

			viewerPagePanels.add(panel);
		}

		SwingUtilities.invokeLater(() -> {
			gotoPage = true;
			revalidate();
			scrollMap.genMap(1.0);
		});
	}

	private void lockSize() {
		Dimension size = getSize();
		setMinimumSize(size);
		setMaximumSize(size);
		//setResizable(false);
	}

	private void unlockSize() {
		setMinimumSize(min);
		setMaximumSize(max);
		//setResizable(true);
	}

	/**
	 * Assume that we are on a certain page by setting the page number field.
	 *
	 * @param pageNum A page number.
	 */
	private void assumePage(int pageNum) {
		this.pageNumberField.setText(new StringBuilder().append(pageNum).toString());
	}

	/**
	 * Assume that we have a certain max page count by setting the max pages field.
	 *
	 * @param pageCount A page count.
	 */
	private void assumeMaxPages(int pageCount) {
		this.maxPagesLabel.setText(new StringBuilder().append(pageCount).toString());
	}
}
