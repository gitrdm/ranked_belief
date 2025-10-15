# GitHub Actions Workflows

## Deploy Doxygen Documentation (`deploy-docs.yml`)

This workflow automatically builds and deploys Doxygen documentation to GitHub Pages.

### Triggers

- **Push to `master` branch**: Automatically rebuilds and deploys docs on every push
- **Manual workflow dispatch**: Can be triggered manually from the Actions tab

### What it does

1. **Build Job**:
   - Checks out the repository
   - Installs Doxygen and Graphviz
   - Runs `doxygen Doxyfile` to generate documentation
   - Uploads the generated HTML as a GitHub Pages artifact

2. **Deploy Job**:
   - Deploys the documentation to GitHub Pages
   - Makes it available at `https://<username>.github.io/<repo>/`

### First-time Setup

To enable GitHub Pages for this repository:

1. Go to **Settings** → **Pages** in your GitHub repository
2. Under **Source**, select **GitHub Actions**
3. Push this workflow file to the `master` branch
4. The workflow will run automatically and deploy the docs

### Viewing the Documentation

After the workflow completes successfully, your documentation will be available at:
- `https://gitrdm.github.io/ranked_belief/`

### Local Testing

To verify the documentation builds correctly before pushing:

```bash
make docs
make show-docs
```

### Troubleshooting

If the deployment fails:
- Check that GitHub Pages is enabled in repository settings
- Ensure the `GITHUB_TOKEN` has sufficient permissions
- Review the workflow logs in the Actions tab
- Verify that `doxygen Doxyfile` runs successfully locally
