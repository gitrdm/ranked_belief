# GitHub Pages Setup Instructions

This repository is configured to automatically publish Doxygen documentation to GitHub Pages.

## One-Time Setup Required

Before the documentation can be published, you need to enable GitHub Pages in your repository settings:

### Steps:

1. **Push this commit to GitHub**:
   ```bash
   git push origin master
   ```

2. **Enable GitHub Pages**:
   - Go to your repository on GitHub: `https://github.com/gitrdm/ranked_belief`
   - Click **Settings** (in the repository menu)
   - Click **Pages** (in the left sidebar)
   - Under **Source**, select **GitHub Actions** from the dropdown
   - Click **Save**

3. **Trigger the workflow**:
   - The workflow will automatically run on the next push to `master`
   - Or trigger it manually:
     - Go to **Actions** tab
     - Click **Deploy Doxygen Documentation to GitHub Pages**
     - Click **Run workflow** → **Run workflow**

4. **Wait for deployment**:
   - The workflow takes about 2-3 minutes to complete
   - Watch the progress in the **Actions** tab
   - Green checkmark = successful deployment

5. **Access your documentation**:
   - Once deployed, visit: `https://gitrdm.github.io/ranked_belief/`
   - The link may take a few minutes to become active after first deployment

## Automatic Updates

After the initial setup, documentation will automatically rebuild and deploy whenever you:
- Push commits to the `master` branch
- The workflow detects changes and redeploys within 2-3 minutes

## Manual Deployment

To manually trigger a documentation rebuild without pushing new commits:

1. Go to **Actions** tab on GitHub
2. Select **Deploy Doxygen Documentation to GitHub Pages**
3. Click **Run workflow** button
4. Select the `master` branch
5. Click **Run workflow**

## Troubleshooting

### Workflow fails with permission error
- Check that GitHub Pages is set to "GitHub Actions" source
- Verify repository settings allow GitHub Actions to deploy Pages

### Documentation doesn't update
- Check the **Actions** tab for failed workflows
- Review workflow logs for Doxygen errors
- Test locally with `make docs` to verify Doxygen builds successfully

### 404 error when accessing docs
- Wait a few minutes after first deployment
- Verify the workflow completed successfully
- Check that Pages is enabled in repository settings

## Local Preview

Before pushing, preview the documentation locally:

```bash
make docs       # Generate documentation
make show-docs  # Open in browser
```

## Badge Status

The README includes workflow status badges:
- 🟢 Green: Documentation successfully deployed
- 🔴 Red: Deployment failed (check Actions tab)
- 🟡 Yellow: Deployment in progress

---

**Next Step**: Push this commit and enable GitHub Pages following the steps above!
